# GUI-based-Discoverer
It is the GUI interface for the discoverer command code implemented for Linux.
Mostly we use gst-discoverer-1.0 command along with the name of the file to know about the metadata of a file.
In this code I have implemented the basic GUI interface for the discoverer command. 
It have options such as choosing the file from our system and also it have a option to place uri of the file stored in google. When we select our file it will print the details of the particular file metadata on the console. 
I have also added an option to clear the whole text view log as well as I also added an option to download the logs which we have searched to be stored in a .txt file.
The version of gtk I used in gtk4.

To compile the code:

    gcc GUI_for_discoverer.c `pkg-config --cflags --libs gstreamer-1.0 gstreamer-pbutils-1.0 gtk4` -o GUI_for_discoverer

To run:

    ./GUI_for_discoverer
