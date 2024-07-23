**Brick Lang** is a compiled programming language inspired by the C programming language that tries to minimize the use of brackets. Initial idea and code inspiration from Pixeled on YouTube ([GitHub](https://github.com/orosmatthew/hydrogen-cpp)). 
View examples/everything.brick for documentation

*Prerequisites:* 
- You need `masm32` installed on your computer under `C:\masm32`.
- Additionally, you need g++ that supports up to c++ 17

*Building*
- In the directory with all the *.cpp run `g++ preprocessor.cpp tokenization.cpp parsing.cpp generation.cpp main.cpp -std=c++17 -o brick.exe`


*Note:* 
- Currently, there is only support for Windows 10, but feel free to write a pull request for other operating systems.

*Development status:*
- Due to Windows not working anymore on my computer, I have shifted to Ubuntu and since I do not want to rewrite the entire generation code right now, the development of this compiler is no longer active

