#!/usr/bin/env python3
import os
import re
import subprocess
import tempfile
import time
import pytest
from datetime import datetime
from pathlib import Path

def build_demo():
    demo_code = r'''
    #include "../include/metric_logger.h"
    #include <thread>
    #include <chrono>
    
    void run_single(SimpleMetricWriter& writer) {
        writer.add_metric("CPU", 0.95);
        writer.add_metric("Memory", 512);
        writer.flush();
    }
    
    void run_multi(SimpleMetricWriter& writer) {
        for (int i = 0; i < 3; ++i) {
            writer.add_metric("CPU", 0.1 * i);
            writer.add_metric("Requests", i * 10);
            writer.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    void run_stress(SimpleMetricWriter& writer, int duration_sec) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < 
               std::chrono::seconds(duration_sec)) {
            writer.add_metric("Stress", rand() % 100);
        }
    }
    
    int main(int argc, char** argv) {
        if (argc < 3) return 1;
        
        SimpleMetricWriter writer(argv[1]);
        std::string mode(argv[2]);
        
        if (mode == "single") run_single(writer);
        else if (mode == "multi") run_multi(writer);
        else if (mode == "stress") run_stress(writer, 2);
        
        return 0;
    }
    '''
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.cpp', delete=False) as f:
        f.write(demo_code)
        demo_src = f.name
    
    exe_path = demo_src.replace('.cpp', '')
    subprocess.run([
        'g++', demo_src, '-I.', '-std=c++11', '-pthread', '-o', exe_path
    ], check=True)
    
    os.unlink(demo_src)
    return exe_path

def parse_metrics_file(file_path):
    metrics = []
    with open(file_path) as f:
        for line in f:
            matches = re.findall(r'"([^"]+)"\s+([^\s]+)', line)
            metrics.extend({'name': m[0], 'value': m[1]} for m in matches)
    return metrics

@pytest.fixture(scope='module')
def demo_binary():
    binary = build_demo()
    yield binary
    os.unlink(binary)

@pytest.fixture
def log_file(tmp_path):
    return tmp_path / 'metrics.log'

# Тесты
def test_single_metric(log_file, demo_binary):
    subprocess.run([demo_binary, str(log_file), 'single'], check=True)
    
    metrics = parse_metrics_file(log_file)
    assert len(metrics) == 2
    assert metrics[0]['name'] == 'CPU'
    assert metrics[0]['value'] == '0.95'
    assert metrics[1]['name'] == 'Memory'
    assert metrics[1]['value'] == '512'

def test_multiple_writes(log_file, demo_binary):
    subprocess.run([demo_binary, str(log_file), 'multi'], check=True)
    
    metrics = parse_metrics_file(log_file)
    assert len(metrics) == 6  #3 итерации 2 метрики
    assert all(float(m['value']) >= 0 for m in metrics if m['name'] == 'CPU')

def test_error_handling(tmp_path, demo_binary):
    result = subprocess.run(
        [demo_binary, "", "single"],
        capture_output=True,
        text=True
    )
    assert result.returncode != 0
    assert "cannot be empty" in result.stderr
    
    bad_path = tmp_path / "nonexistent_dir" / "test.log"
    result = subprocess.run(
        [demo_binary, str(bad_path), "single"],
        capture_output=True,
        text=True
    )
    assert result.returncode != 0

if __name__ == "__main__":
    pytest.main(["-v", __file__])