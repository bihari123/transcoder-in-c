# low latency transcoder-in-c

# Low-Latency HLS Video Transcoder

A high-performance, multi-quality video transcoder designed for low-latency HTTP Live Streaming (LL-HLS). The transcoder captures video from a V4L2 device (in this case, a local webcam), transcodes it into multiple quality levels (ffmpeg, h264), and generates HLS playlists for adaptive bitrate streaming (web client).

## Features

- **Low-Latency HLS Support**

  - 200ms partial segments
  - 1-second segments
  - Automatic playlist management
  - FMP4 segment format

- **Multi-Quality Transcoding**

  - 1080p (6 Mbps)
  - 720p (3.5 Mbps)
  - 480p (1.5 Mbps)
  - Adaptive bitrate streaming support

- **Performance Optimizations**
  - Multi-threaded design
  - Ring buffer management
  - Efficient frame pacing
  - Zero-copy where possible
  - Memory-efficient buffer management

## Prerequisites

- FFmpeg development libraries (4.2+)
- GCC or Clang compiler
- Make build system
- Linux system with V4L2 support
- Webcam supporting MJPEG format

### Dependencies Installation

```bash
# Ubuntu/Debian
make deps

# Manual installation
sudo apt-get install \
    build-essential \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libavdevice-dev \
    libavfilter-dev \
    libswscale-dev
```

## Project Structure

```
transcoder/
├── include/           # Header files
│   ├── buffer.h      # Buffer management
│   ├── config.h      # Global configuration
│   ├── cleanup.h     # Resource cleanup
│   ├── decoder.h     # Video decoding
│   ├── encoder.h     # Video encoding
│   ├── monitor.h     # Performance monitoring
│   ├── presets.h     # Quality presets
│   ├── processor.h   # Frame processing
│   ├── types.h       # Data structures
│   └── utils.h       # Utility functions
├── src/              # Implementation files
│   ├── buffer.c
│   ├── cleanup.c
│   ├── decoder.c
│   ├── encoder.c
│   ├── main.c
│   ├── monitor.c
│   ├── presets.c
│   ├── processor.c
│   └── utils.c
├── build/            # Build artifacts
└── Makefile
```

## Build Instructions

```bash
# Standard build
make

# Debug build with additional logging
make debug

# Clean build artifacts
make clean

# Show all make targets
make help
```

## Configuration

Key parameters in `include/config.h`:

```c
#define PART_DURATION 0.2         // 200ms parts for LL-HLS
#define SEGMENT_DURATION 1        // 1 second segments
#define MAX_SEGMENTS_IN_LIST 6    // Segments in playlist
#define BUFFER_SIZE (8192 * 1024) // 8MB buffer
#define GOP_SIZE 60               // GOP size for keyframes
#define MAX_QUALITY_LEVELS 3      // Number of qualities
#define MONITORING_INTERVAL 1     // Stats update interval
#define DEBUG_MODE 1              // Enable debug output
```

Quality presets in `include/presets.h`:

- 1080p: 1920x1080 @ 30fps, 6 Mbps
- 720p: 1280x720 @ 30fps, 3.5 Mbps
- 480p: 854x480 @ 30fps, 1.5 Mbps

## Usage

```bash
# Create output directory
mkdir -p stream_output

# Run transcoder
./transcoder stream_output

# go into another terminal and cd into stream_output dir
python -m http.server 8080

# go to localhost:8080 to see the live feed

# Or Play stream (using ffplay)
ffplay -fflags nobuffer -flags low_delay stream_output/master.m3u8
```

## Technical Details

### Video Pipeline

1. **Capture**

   - V4L2 device capture
   - MJPEG format input
   - Native camera framerate

2. **Decoding**

   - Hardware-accelerated MJPEG decoding
   - Frame timestamp management
   - Frame rate control

3. **Processing**

   - Frame scaling for each quality level
   - Buffer management
   - Frame dropping when necessary

4. **Encoding**

   - H.264 encoding (x264)
   - Ultrafast preset for low latency
   - Zero-latency tuning
   - GOP alignment
   - Constant bitrate encoding

5. **Packaging**
   - FMP4 segmentation
   - LL-HLS playlist generation
   - Multi-quality manifest

### Performance Monitoring

- Real-time statistics

  - Dropped frames
  - Encoding quality
  - Buffer status
  - Latency metrics

- Debug logging
  - Packet timing
  - Frame processing
  - Error conditions

## Error Handling

- Input device failures
- Buffer overflow conditions
- Encoding errors
- File system errors
- Resource cleanup

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Troubleshooting

1. **Video Device Access**

   ```bash
   sudo chmod 666 /dev/video0
   ```

2. **Check Camera Formats**

   ```bash
   v4l2-ctl --list-formats
   ```

3. **Monitor System Resources**

   ```bash
   htop
   ```

4. **Debug Output**

   ```bash
   DEBUG_MODE=1 ./transcoder stream_output
   ```

5. **Common Issues**
   - Insufficient permissions
   - Unsupported camera format
   - Insufficient system resources
   - Network congestion
   - File system space

## Performance Tuning

1. **Buffer Size**

   - Adjust `BUFFER_SIZE` for memory usage
   - Monitor buffer overflow conditions

2. **Quality Levels**

   - Modify bitrates in presets
   - Adjust resolution scaling

3. **Segment Duration**

   - Balance latency vs stability
   - Adjust `PART_DURATION` and `SEGMENT_DURATION`

4. **GOP Size**
   - Impact on latency and quality
   - Modify `GOP_SIZE` based on needs

## Support

For issues and feature requests, please create an issue in the repository.
