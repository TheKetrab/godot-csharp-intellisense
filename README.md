# Godot Editor C# IntelliSense

<p align="center">
<img src="https://upload.wikimedia.org/wikipedia/commons/thumb/5/5a/Godot_logo.svg/320px-Godot_logo.svg.png" />
</p>

This project is an implementation of autocompletion feature for C# language. Made for thesis by Bartłomiej Grochowski, UWR 2020. This repository consists of C++ sources that you can compile into Godot Mono version

## Content of the repository
The repository contains:
* special programs to simulate work of IntelliSense outside Godot Engine
* necessary files that you have to include to your Godot workspace directory to successfully compile IntelliSense
* thesis papers that explain how the IntelliSense module works

## Compile into Godot

1. Download Godot source code from the last stable release version, eg:
```
git clone https://github.com/godotengine/godot.git −−single−branch −−branch 3.2
```

2. Compile Mono version following the <a target="_blank" href="https://docs.godotengine.org/en/stable/development/compiling/index.html">official tutorial</a>.

3. Copy the folowing files to `.../godot/modules/mono` directory:
* CSharpIntellisense/csharp_lexer.h
* CSharpIntellisense/csharp_lexer.cpp
* CSharpIntellisense/csharp_parser.h
* CSharpIntellisense/csharp_parser.cpp
* CSharpIntellisense/csharp_context.h
* CSharpIntellisense/csharp_context.cpp
* CSharpIntellisense/csharp_utils.h
* CSharpIntellisense/csharp_utils.cpp
* ExternalGodotFiles/csharp_provider_mono.h
* ExternalGodotFiles/csharp_provider_mono.cpp 

4. Copy the definition of function `CSharpLanguage::complete_code` from `ExternalGodotFiles/csharp_script.cpp` to `.../godot/modules/mono/csharp_script.cpp`.

5. Recompile Godot with Mono and run it.

<a target="_blank" href="https://i.imgur.com/HvjIP1J.jpg"><img src="https://i.imgur.com/HvjIP1J.jpg" width="600"></a>


## LiveIntellisense demo (Windows only)
Solution file CSharpIntellisense.sln consists of three projects. Compile each of them - they should compile into `Output` directory. Put the `Input` directory inside and add some .cs files there. To simulate cursor placement type `^|` and wait for the LiveIntellisense output. Note: CSharpIntellisense.exe, AssemblyReader.exe and LiveIntellisense.exe must be in the same directories.

<a target="_blank" href="https://i.imgur.com/hmwaldy.jpg"><img src="https://i.imgur.com/hmwaldy.jpg" width="600"></a>
