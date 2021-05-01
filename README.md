# fcgi-client-cpp
A simple C++ library for quick development fcgi client application.

<!-- ABOUT THE PROJECT -->
## About The Project

This is a small C++ library for quick development on fcgi client applications. Its built-in connectivitiy functionality is based on asio c++ library, which can deal with either tcp or domain socket stream connection to fcgi servers.

### Built With

This project is based on asio c++ library and cmake tool
* [asio](https://think-async.com/Asio/)
* [cmake](https://cmake.org/download/)

Or they can be installed with ubuntu packages:
* apt-get install libasio-dev
* apt-get install cmake

<!-- GETTING STARTED -->
### Installation
git clone https://github.com/usb-tech07/fcgi-client-cpp.git  
cd fcgi-client-cpp  
mkdir build  
cd build  
cmake ..  
make && make install  

<!-- USAGE EXAMPLES -->
## Usage
In example directory, there is a sample program showing how to use the generated library.

<!-- LICENSE -->
## License

Distributed under the BSD License. See `LICENSE` for more information.

<!-- CONTACT -->
## Contact

Vincent Yin - haiyang_yin@yahoo.ca

Project Link: [https://github.com/usb-tech07/fcgi-client-cpp](https://github.com/usb-tech07/fcgi-client-cpp)

<!-- ACKNOWLEDGEMENTS -->
## Acknowledgements
* [GitHub Best-README-Template](https://github.com/othneildrew/Best-README-Template)

