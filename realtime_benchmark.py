#!/usr/bin/env python3

# Check required packages
try:
    import matplotlib
    matplotlib.use('TkAgg')  # Use TkAgg backend
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    import numpy as np
    import psutil
except ImportError as e:
    print(f"Required package missing: {e}")
    print("Please install required packages:")
    print("pip3 install matplotlib numpy psutil")
    exit(1)

import time
import threading
import json
import signal
import sys
from collections import deque
import hashlib
import queue
from concurrent.futures import ThreadPoolExecutor
import uuid
import os

class RealtimeBenchmark:
    def __init__(self):
        self.target_tps = 1_000_000
        self.current_tps = 0
        self.peak_tps = 0
        self.running = True
        self.transaction_count = 0
        self.last_count_time = time.time()
        self.start_time = time.time()
        
        # Transaction processing queues and pools
        self.tx_queue = queue.Queue(maxsize=1_000_000)
        self.result_queue = queue.Queue()
        self.thread_pool = ThreadPoolExecutor(max_workers=psutil.cpu_count())
        self.processing_threads = []
        
        # Use deques with max length for data storage
        self.max_points = 600
        self.timestamps = deque(maxlen=self.max_points)
        self.tps_values = deque(maxlen=self.max_points)
        self.cpu_usage = deque(maxlen=self.max_points)
        self.memory_usage = deque(maxlen=self.max_points)
        
        # Performance parameters
        self.params = {
            'batch_size': 10000,
            'num_threads': psutil.cpu_count(),
            'parallel_chains': 32,
            'memory_pool_size': 10_000_000,
        }
        
        # Adjust batch size based on target TPS
        min_batch = 5000
        max_batch = 50000
        self.params['batch_size'] = min(max_batch, max(min_batch, self.target_tps // 100))
        
        # Setup signal handler
        signal.signal(signal.SIGINT, self.signal_handler)
        
        # Initialize plots
        self.setup_plots()

    def signal_handler(self, signum, frame):
        print("\nShutting down gracefully...")
        self.stop_benchmark()
        
    def setup_plots(self):
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(12, 8))
        
        # TPS plot
        self.tps_line, = self.ax1.plot([], [], 'b-', label='Current TPS')
        self.ax1.set_title('Transactions Per Second')
        self.ax1.set_xlabel('Time (s)')
        self.ax1.set_ylabel('TPS')
        self.ax1.grid(True)
        self.ax1.legend()
        
        # Resource usage plot
        self.cpu_line, = self.ax2.plot([], [], 'r-', label='CPU Usage')
        self.memory_line, = self.ax2.plot([], [], 'g-', label='Memory Usage')
        self.ax2.set_title('Resource Usage')
        self.ax2.set_xlabel('Time (s)')
        self.ax2.set_ylabel('Percentage')
        self.ax2.grid(True)
        self.ax2.legend()
        
        plt.tight_layout()
        
    def update_plot(self, frame):
        try:
            # Calculate elapsed time
            current_time = time.time() - self.start_time
            
            # Add new data points
            self.timestamps.append(current_time)
            self.tps_values.append(self.current_tps)
            self.cpu_usage.append(psutil.cpu_percent())
            self.memory_usage.append(psutil.virtual_memory().percent)
            
            # Update TPS plot
            self.tps_line.set_data(list(self.timestamps), list(self.tps_values))
            self.ax1.relim()
            self.ax1.autoscale_view()
            
            # Set reasonable TPS range
            max_tps = max(max(self.tps_values, default=0), 1)
            target_line = min(max_tps * 1.2, self.target_tps * 1.2)
            self.ax1.set_ylim(0, target_line)
            
            # Add target TPS line if within view
            if not hasattr(self, 'target_line'):
                self.target_line = self.ax1.axhline(y=self.target_tps, color='r', linestyle='--', alpha=0.5)
            
            # Update resource usage plot
            self.cpu_line.set_data(list(self.timestamps), list(self.cpu_usage))
            self.memory_line.set_data(list(self.timestamps), list(self.memory_usage))
            self.ax2.relim()
            self.ax2.autoscale_view()
            self.ax2.set_ylim(0, 100)
            
            # Keep x-axis showing last 60 seconds
            if self.timestamps:
                x_max = max(self.timestamps)
                self.ax1.set_xlim(max(0, x_max - 60), x_max)
                self.ax2.set_xlim(max(0, x_max - 60), x_max)
            
            return self.tps_line, self.cpu_line, self.memory_line, self.target_line
            
        except Exception as e:
            print(f"Error updating plot: {e}")
            return self.tps_line, self.cpu_line, self.memory_line, self.target_line
    
    def process_transaction(self, tx_data):
        try:
            # Simulate actual blockchain work
            # 1. Hash the transaction
            tx_hash = hashlib.blake2b(tx_data).digest()
            
            # 2. Verify digital signature (simulated)
            sig_verify = hashlib.sha3_256(tx_hash).digest()
            
            # 3. State transition computation
            state_update = hashlib.sha3_512(sig_verify + tx_hash).digest()
            
            # 4. Merkle tree update (simulated)
            merkle_update = hashlib.blake2s(state_update).digest()
            
            return True
        except Exception as e:
            print(f"Transaction processing error: {e}")
            return False

    def process_batch(self, batch):
        results = []
        for tx in batch:
            result = self.process_transaction(tx)
            results.append(result)
        return results

    def transaction_processor(self):
        while self.running:
            try:
                # Get a batch of transactions
                batch = []
                try:
                    while len(batch) < self.params['batch_size'] and not self.tx_queue.empty():
                        batch.append(self.tx_queue.get_nowait())
                except queue.Empty:
                    if not batch:
                        time.sleep(0.0001)
                        continue

                # Process the batch
                if batch:
                    results = self.process_batch(batch)
                    self.result_queue.put(len([r for r in results if r]))
            except Exception as e:
                print(f"Processor thread error: {e}")
                if not self.running:
                    break

    def submit_transactions(self):
        last_batch_time = time.time()
        last_tps_update = time.time()
        accumulated_tx = 0
        
        # Start processing threads
        for _ in range(self.params['num_threads']):
            thread = threading.Thread(target=self.transaction_processor)
            thread.daemon = True
            thread.start()
            self.processing_threads.append(thread)
        
        # Calculate base batch interval
        batch_interval = 1.0 / (self.target_tps / self.params['batch_size'])
        min_interval = 0.0001
        
        while self.running:
            try:
                current_time = time.time()
                elapsed = current_time - last_batch_time
                
                if elapsed >= max(batch_interval, min_interval):
                    # Generate and submit real transactions
                    batch_size = self.params['batch_size']
                    for _ in range(batch_size):
                        # Create transaction with real data
                        tx_data = os.urandom(256)  # Random 256 bytes
                        try:
                            self.tx_queue.put_nowait(tx_data)
                        except queue.Full:
                            time.sleep(0.0001)
                            continue
                    
                    accumulated_tx += batch_size
                    self.transaction_count += batch_size
                    
                    # Process results
                    try:
                        while not self.result_queue.empty():
                            processed = self.result_queue.get_nowait()
                            if processed > 0:
                                accumulated_tx += processed
                    except queue.Empty:
                        pass
                    
                    # Update TPS every 100ms
                    if current_time - last_tps_update >= 0.1:
                        actual_window = current_time - last_tps_update
                        self.current_tps = int(accumulated_tx / actual_window)
                        self.peak_tps = max(self.peak_tps, self.current_tps)
                        accumulated_tx = 0
                        last_tps_update = current_time
                    
                    # Dynamic sleep calculation
                    processing_time = time.time() - current_time
                    sleep_time = max(0, batch_interval - processing_time)
                    if sleep_time > 0:
                        time.sleep(sleep_time)
                    
                    last_batch_time = time.time()
                else:
                    time.sleep(0.0001)
                    
            except Exception as e:
                print(f"Error in transaction submission: {e}")
                if not self.running:
                    break

    def run_benchmark(self):
        print(f"Starting benchmark with target TPS: {self.target_tps}")
        print("Initial parameters:", json.dumps(self.params, indent=2))
        print("Benchmark running. Press Ctrl+C to stop...")
        
        try:
            # Start transaction submission in a separate thread
            self.tx_thread = threading.Thread(target=self.submit_transactions)
            self.tx_thread.daemon = True
            self.tx_thread.start()
            
            # Create animation
            self.anim = animation.FuncAnimation(
                self.fig,
                self.update_plot,
                frames=self.generate_time(),
                interval=100,  # Update every 100ms
                blit=True,
                cache_frame_data=False  # Disable frame caching
            )
            
            # Show plot (this blocks until window is closed)
            plt.show()
            
        except KeyboardInterrupt:
            print("\nReceived keyboard interrupt. Stopping benchmark...")
        finally:
            self.cleanup()
    
    def generate_time(self):
        t = 0
        while self.running:
            yield t
            t += 0.1  # 100ms between frames
    
    def stop_benchmark(self):
        self.running = False
        if hasattr(self, 'tx_thread'):
            self.tx_thread.join(timeout=1.0)
    
    def cleanup(self):
        print("Cleaning up...")
        try:
            self.running = False
            
            # Wait for processing threads to finish
            for thread in self.processing_threads:
                thread.join(timeout=1.0)
            
            # Shutdown thread pool
            self.thread_pool.shutdown(wait=False)
            
            # Save results
            results = {
                'peak_tps': self.peak_tps,
                'avg_tps': float(np.mean(list(self.tps_values))) if self.tps_values else 0,
                'final_params': self.params,
                'processed_transactions': self.transaction_count
            }
            
            with open('benchmark_results.json', 'w') as f:
                json.dump(results, f, indent=2)
            print("Results saved to benchmark_results.json")
            
            # Close plots
            plt.close('all')
            
        except Exception as e:
            print(f"Error during cleanup: {e}")

if __name__ == "__main__":
    try:
        benchmark = RealtimeBenchmark()
        benchmark.run_benchmark()
    except Exception as e:
        print(f"Benchmark failed: {e}")
        sys.exit(1) 