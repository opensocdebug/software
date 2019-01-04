This repository hosts the *OLD* and *ABANDONED* code of the Open SoC Debug host software.
The current host software is available at https://github.com/opensocdebug/osd-sw.

----------

This is the host software library and daemon for the Open SoC Debug
project.

It is work in progress, please visit http://opensocdebug.org for more
information.

## Try it out

* Build the software: `./autogen.sh; cd build; ../configure; make; make install`
* Start a simulation or board
* Connect the daemon: `opensocdebugd tcp`
* It will start an xterm for the terminal
* Connect to the daemon using the command line interface: `osd-cli`
* Reset the system and keep cores halted: `reset -halt`
* Start the processor cores: `start`
* Run memory test: `mem test`
