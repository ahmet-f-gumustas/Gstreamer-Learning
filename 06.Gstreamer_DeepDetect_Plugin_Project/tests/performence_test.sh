#!/bin/bash
# Performance tests for DeepDetect plugin

set -e

echo "========================================"
echo "DeepDetect Plugin Performance Tests"
echo "========================================"
echo ""

# Configuration
NUM_FRAMES=1000
RESOLUTIONS=("640x480" "1280x720" "1920x1080")
FORMATS=("RGB" "NV12" "RGBA")

# Function to run performance test
run_performance_test() {
    local resolution=$1
    local format=$2
    local num_frames=$3
    
    echo "Testing: ${resolution} ${format} (${num_frames} frames)"
    echo "----------------------------------------------------"
    
    start_time=$(date +%s.%N)
    
    timeout 60s gst-launch-1.0 \
        videotestsrc num-buffers=$num_frames pattern=ball ! \
        "video/x-raw,format=${format},width=${resolution%x*},height=${resolution#*x},framerate=30/1" ! \
        identity ! \
        fakesink sync=false > /dev/null 2>&1
    
    end_time=$(date +%s.%N)
    duration=$(echo "$end_time - $start_time" | bc -l)
    fps=$(echo "scale=2; $num_frames / $duration" | bc -l)
    
    echo "Duration: ${duration}s"
    echo "FPS: ${fps}"
    echo ""
    
    # Store results
    echo "${resolution},${format},${num_frames},${duration},${fps}" >> performance_results.csv
}

# Function to test memory usage
test_memory_usage() {
    echo "Memory Usage Test"
    echo "=================="
    
    local test_duration=30
    local resolution="1280x720"
    
    echo "Monitoring memory usage for ${test_duration}s..."
    
    # Start memory monitoring in background
    (
        while true; do
            memory_mb=$(free -m | awk '/^Mem:/{print $3}')
            timestamp=$(date +%s.%N)
            echo "$timestamp,$memory_mb" >> memory_usage.csv
            sleep 0.5
        done
    ) &
    monitor_pid=$!
    
    # Run test pipeline
    timeout ${test_duration}s gst-launch-1.0 \
        videotestsrc pattern=ball ! \
        "video/x-raw,format=RGB,width=${resolution%x*},height=${resolution#*x},framerate=30/1" ! \
        identity ! \
        fakesink sync=false > /dev/null 2>&1 || true
    
    # Stop monitoring
    kill $monitor_pid 2>/dev/null || true
    wait $monitor_pid 2>/dev/null || true
    
    # Analyze memory usage
    if [ -f memory_usage.csv ]; then
        min_memory=$(awk -F',' 'NR==1{min=$2} {if($2<min) min=$2} END{print min}' memory_usage.csv)
        max_memory=$(awk -F',' '{if($2>max) max=$2} END{print max}' memory_usage.csv)
        avg_memory=$(awk -F',' '{sum+=$2; count++} END{print sum/count}' memory_usage.csv)
        
        memory_growth=$((max_memory - min_memory))
        
        echo "Memory Usage Analysis:"
        echo "  Minimum: ${min_memory} MB"
        echo "  Maximum: ${max_memory} MB"
        echo "  Average: ${avg_memory} MB"
        echo "  Growth: ${memory_growth} MB"
        
        if [ $memory_growth -lt 50 ]; then
            echo "  ✓ Memory usage stable"
        else
            echo "  ⚠ High memory growth detected"
        fi
        
        rm -f memory_usage.csv
    fi
    echo ""
}

# Function to test CPU usage
test_cpu_usage() {
    echo "CPU Usage Test"
    echo "=============="
    
    local test_duration=20
    
    echo "Monitoring CPU usage for ${test_duration}s..."
    
    # Get initial CPU stats
    cpu_before=$(grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$3+$4)} END {print usage}')
    
    # Run test pipeline
    timeout ${test_duration}s gst-launch-1.0 \
        videotestsrc pattern=ball ! \
        'video/x-raw,format=RGB,width=1280,height=720,framerate=30/1' ! \
        identity ! \
        fakesink sync=false > /dev/null 2>&1 || true
    
    # Get final CPU stats
    cpu_after=$(grep 'cpu ' /proc/stat | awk '{usage=($2+$4)*100/($2+$3+$4)} END {print usage}')
    
    cpu_usage=$(echo "$cpu_after - $cpu_before" | bc -l)
    
    echo "CPU Usage: ${cpu_usage}%"
    
    if (( $(echo "$cpu_usage < 80" | bc -l) )); then
        echo "✓ CPU usage acceptable"
    else
        echo "⚠ High CPU usage"
    fi
    echo ""
}

# Function to test pipeline latency
test_latency() {
    echo "Latency Test"
    echo "============"
    
    echo "Measuring pipeline latency..."
    
    # Create latency test pipeline
    python3 << 'EOF'
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib
import time
import statistics

Gst.init(None)

latencies = []

def on_buffer_probe(pad, info, user_data):
    buffer = info.get_buffer()
    if buffer:
        # Get timestamp
        pts = buffer.pts
        current_time = time.time_ns()
        
        # Calculate latency (this is a simplified measurement)
        latency_ms = (current_time - pts) / 1000000  # Convert to milliseconds
        latencies.append(latency_ms)
        
        if len(latencies) >= 100:  # Stop after 100 measurements
            user_data['loop'].quit()
    
    return Gst.PadProbeReturn.OK

# Create pipeline
pipeline = Gst.parse_launch('''
    videotestsrc num-buffers=100 !
    video/x-raw,format=RGB,width=640,height=480,framerate=30/1 !
    identity name=probe !
    fakesink sync=false
''')

# Add probe
identity = pipeline.get_by_name('probe')
src_pad = identity.get_static_pad('src')

loop = GLib.MainLoop()
src_pad.add_probe(Gst.PadProbeType.BUFFER, on_buffer_probe, {'loop': loop})

# Run pipeline
pipeline.set_state(Gst.State.PLAYING)
loop.run()
pipeline.set_state(Gst.State.NULL)

# Calculate statistics
if latencies:
    avg_latency = statistics.mean(latencies)
    min_latency = min(latencies)
    max_latency = max(latencies)
    std_latency = statistics.stdev(latencies) if len(latencies) > 1 else 0
    
    print(f"Latency Statistics ({len(latencies)} samples):")
    print(f"  Average: {avg_latency:.2f} ms")
    print(f"  Minimum: {min_latency:.2f} ms")
    print(f"  Maximum: {max_latency:.2f} ms")
    print(f"  Std Dev: {std_latency:.2f} ms")
    
    if avg_latency < 50:
        print("  ✓ Latency acceptable")
    else:
        print("  ⚠ High latency detected")
else:
    print("No latency measurements collected")
EOF
    echo ""
}

# Function to test throughput scaling
test_throughput_scaling() {
    echo "Throughput Scaling Test"
    echo "======================"
    
    local base_fps=30
    local framerates=(15 30 60 120)
    
    echo "Testing throughput at different framerates..."
    
    for framerate in "${framerates[@]}"; do
        echo "Testing at ${framerate} FPS..."
        
        start_time=$(date +%s.%N)
        
        timeout 15s gst-launch-1.0 \
            videotestsrc num-buffers=$((framerate * 10)) ! \
            "video/x-raw,format=RGB,width=640,height=480,framerate=${framerate}/1" ! \
            identity ! \
            fakesink sync=false > /dev/null 2>&1 || true
        
        end_time=$(date +%s.%N)
        duration=$(echo "$end_time - $start_time" | bc -l)
        achieved_fps=$(echo "scale=2; $((framerate * 10)) / $duration" | bc -l)
        efficiency=$(echo "scale=2; $achieved_fps / $framerate * 100" | bc -l)
        
        echo "  Target: ${framerate} FPS"
        echo "  Achieved: ${achieved_fps} FPS"
        echo "  Efficiency: ${efficiency}%"
        echo ""
    done
}

# Function to generate performance report
generate_report() {
    echo "Performance Report Generation"
    echo "============================"
    
    local report_file="performance_report.txt"
    
    cat > "$report_file" << EOF
DeepDetect Plugin Performance Report
Generated: $(date)

System Information:
- CPU: $(lscpu | grep "Model name" | awk -F: '{print $2}' | xargs)
- Cores: $(nproc)
- Memory: $(free -h | awk '/^Mem:/{print $2}')
- GPU: $(nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null || echo "Not available")

Test Results:
EOF
    
    if [ -f performance_results.csv ]; then
        echo "" >> "$report_file"
        echo "Throughput Tests:" >> "$report_file"
        echo "Resolution,Format,Frames,Duration(s),FPS" >> "$report_file"
        cat performance_results.csv >> "$report_file"
    fi
    
    echo "" >> "$report_file"
    echo "Recommendations:" >> "$report_file"
    echo "- For optimal performance, use NV12 format with GPU memory" >> "$report_file"
    echo "- Consider using FP16 precision for better throughput" >> "$report_file"
    echo "- Monitor memory usage in production deployments" >> "$report_file"
    
    echo "Report saved to: $report_file"
}

# Main test execution
main() {
    echo "Starting performance tests..."
    echo ""
    
    # Initialize results file
    echo "resolution,format,frames,duration,fps" > performance_results.csv
    
    # Run basic throughput tests
    echo "Basic Throughput Tests"
    echo "====================="
    for resolution in "${RESOLUTIONS[@]}"; do
        for format in "${FORMATS[@]}"; do
            run_performance_test "$resolution" "$format" 300
        done
    done
    
    # Run memory usage test
    test_memory_usage
    
    # Run CPU usage test  
    test_cpu_usage
    
    # Run latency test
    test_latency
    
    # Run throughput scaling test
    test_throughput_scaling
    
    # Generate report
    generate_report
    
    echo "========================================"
    echo "Performance tests completed!"
    echo "========================================"
    
    # Cleanup
    rm -f performance_results.csv
}

# Check dependencies
if ! command -v bc &> /dev/null; then
    echo "Error: 'bc' calculator not found. Please install it."
    exit 1
fi

# Run main function
main