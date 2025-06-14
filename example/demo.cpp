#include "metric_logger.h"

void* cpu_metric_thread(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;
  while (true) {
    double cpu_load = (double)rand() / RAND_MAX * 4.0;
    args->writer->add_metric("CPU", cpu_load);
    usleep(200000);
  }
  return NULL;
}

void* http_metric_thread(void* arg) {
  ThreadArgs* args = (ThreadArgs*)arg;
  int count = 0;
  while (true) {
    args->writer->add_metric("HTTP Requests", ++count);
    usleep(300000);
  }
  return NULL;
}

int main() {
  try {
    std::string log_path = std::string(PROJECT_ROOT_DIR) + "/log.txt";
    SimpleMetricWriter writer(log_path);

    ThreadArgs cpu_args, http_args;
    cpu_args.writer = &writer;
    http_args.writer = &writer;

    pthread_t cpu_thread, http_thread;
    pthread_create(&cpu_thread, NULL, cpu_metric_thread, &cpu_args);
    pthread_create(&http_thread, NULL, http_metric_thread, &http_args);

    while (true) {
      sleep(1);
      writer.flush();
    }

    pthread_join(cpu_thread, NULL);
    pthread_join(http_thread, NULL);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}