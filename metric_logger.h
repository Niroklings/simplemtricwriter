#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <pthread.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>

class SimpleMetricWriter {
public:
    SimpleMetricWriter(const std::string& filename) : filename(filename) {
        if (filename.empty()) {
            throw std::runtime_error("Filename cannot be empty");
        }
        pthread_mutex_init(&mtx, NULL);
    }

    ~SimpleMetricWriter() {
        pthread_mutex_destroy(&mtx);
    }

    void add_metric(const std::string& name, const std::string& value) {
        pthread_mutex_lock(&mtx);
        Metric m;
        m.name = name;
        m.value = value;
        metrics.push_back(m);
        pthread_mutex_unlock(&mtx);
    }

    template<typename T>
    void add_metric(const std::string& name, T value) {
        std::ostringstream oss;
        oss << value;
        add_metric(name, oss.str());
    }

    void flush() {
        pthread_mutex_lock(&mtx);
        if (metrics.empty()) {
            pthread_mutex_unlock(&mtx);
            return;
        }

        std::ofstream file(filename.c_str(), std::ios::app);
        if (!file) {
            pthread_mutex_unlock(&mtx);
            throw std::runtime_error("Failed to open log file");
        }

        file << get_current_timestamp() << " ";
        for (size_t i = 0; i < metrics.size(); ++i) {
            const Metric& m = metrics[i];
            file << "\"" << m.name << "\" " << m.value;
            if (i != metrics.size() - 1) file << " ";
        }
        file << "\n";

        metrics.clear();
        pthread_mutex_unlock(&mtx);
    }

private:
    struct Metric {
        std::string name;
        std::string value;
    };

    std::string filename;
    std::vector<Metric> metrics;
    pthread_mutex_t mtx;

    std::string get_current_timestamp() {
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        struct timeval tv;
        gettimeofday(&tv, NULL);
        int milliseconds = tv.tv_usec / 1000;
        
        std::ostringstream oss;
        oss << buffer << "." << std::setfill('0') << std::setw(3) << milliseconds;
        return oss.str();
    }
};

struct ThreadArgs {
    SimpleMetricWriter* writer;
};
