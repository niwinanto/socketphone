# socketphone

This part of RTOS mid term project.
In this project instead of sending text messages you are going to exchange digitized audio data (un compressed) between two computers using TCP/IP Sockets
The Microphone and Speakers are hardware devices (they can be called as sensors and actuators), these are analog devices. However the audio card on your computer do have Analog to Digital and Digital to analog converter attached to these devices.  Since the audio card is an hardware device we need device drivers to interact with it Since the device drivers are very much hardware dependent, all implementation of Linux has come out with various sets of Audio API which interact with device drivers. These API hides the microscopic low level and hard to learn details  from a programmer and exposes a simple set of API to the programmer.  This makes your learning and implementation much faster.

Make file is included for building the server and client binaries.

Server: Act as the audio recording end.

Client: Act as the audio playback end.
