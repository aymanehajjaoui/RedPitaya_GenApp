# RedPitaya_GenApp

RedPitaya_GenApp is a C++/GTK-based GUI application designed to assist in exporting CNN-based applications to Red Pitaya.  
It features two operation modes:

- **Online Mode**: Clones the latest project template from GitHub during export.
- **Offline Mode**: Uses a bundled local version of the template located under `gen_files/`.

---

## Features

- Graphical user interface using GTKmm
- Model folder selection
- SSH-based deployment to Red Pitaya
- Git clone support (online mode)
- Local export support (offline mode)

---

## 🗂️ Project Structure
``` 
├── Source_Code/ 
│ ├── RedPitaya_GenApp_Online/ # Online version (clones from GitHub) 
│ │ ├── src/ # Main C++ sources 
│ │ ├── include/ # Header files 
│ │ └── Makefile 
│ └── RedPitaya_GenApp_Offline/ # Offline version (uses gen_files/) 
│ ├── src/ 
│ ├── include/ 
│ ├── gen_files/ # Locally bundled RedPitaya template 
│ └── Makefile 
├── RedPitaya_GenApp_Online/ # Compiled binaries and .o files 
├── RedPitaya_GenApp_Offline/ 
└── README.md``` 


---

## 🛠️ Build Instructions

Each version (online and offline) has its own `Makefile`.

### 🔧 Build Online Version
```bash
cd Source_Code/RedPitaya_GenApp_Online
make``` 

### 🔧 Build Online Version
```bash
cd Source_Code/RedPitaya_GenApp_Offline
make``` 
