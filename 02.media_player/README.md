## GStreamer C++ Media Player

This project will create a simple media player application using the GStreamer framework. It will include the following features:

1. Ability to play video and audio files
2. Pause/Resume control
3. Forward/backward skip feature
4. File selection capability
5. Displaying media information (duration, codec, etc.)

### Project Structure

```
gstreamer-cpp-player/
│
├── CMakeLists.txt          # Main CMake configuration file
├── include/                # Header files
│   ├── MediaPlayer.hpp     # MediaPlayer class definition
│   └── Utils.hpp           # Utility functions
│
├── src/                    # Source code
│   ├── main.cpp            # Main application
│   ├── MediaPlayer.cpp     # MediaPlayer class implementation
│   └── Utils.cpp           # Utility functions implementation
│
└── README.md               # Project information
```

```markdown
# GStreamer C++ Media Player

This application is a simple C++ media player using the GStreamer multimedia framework.

## Features

- Playing video and audio files
- Basic player controls (play, pause, stop)
- Forward/backward skip
- Displaying media information

## Requirements

- C++14 compatible compiler
- CMake 3.10 or higher
- GStreamer 1.0 and development packages

## Installation

### Linux

```bash
# Install GStreamer development packages
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-good1.0-dev

# Build the project
mkdir build && cd build
cmake ..
make

# Run
./media-player [file_name]
```

# Run
./media-player.exe [file_name]
```

## Usage

When the program runs, it will display a menu:

1. Open File - Opens a media file
2. Play - Plays the current file
3. Pause - Pauses playback
4. Stop - Stops playback
5. Skip 10 seconds forward - Skips 10 seconds forward
6. Skip 10 seconds backward - Skips 10 seconds backward
7. Seek to a specific position - Seeks to the specified second
8. Show media information - Shows information about the current file
9. Toggle position update - Enables/disables position info updates
0. Exit - Exits the program

## License

This project is distributed under the MIT license.
```

### Building and Running

1. To build the project:
```bash
mkdir build && cd build
cmake ..
make
```

2. To run the program:
```bash
./media-player [file_name]
```

3. Optionally, to install the program:
```bash
sudo make install
```
