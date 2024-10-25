This code is run in Ubuntu WSL, Windows, VS Code

Set Up:
    Install Ubuntu WSL
    In Ubuntu run these commands to install dependencies:
        For CURL:
            sudo apt update
            sudo apt install -y libcurl4-openssl-dev
        For OpenCV:
            sudo apt update
            sudo apt install -y libopencv-dev
        For nlohmann JSON:
            sudo apt update
            sudo apt install -y nlohmann-json3-dev

Compile:
    g++ Main.cpp -o EmergResponse -I/usr/local/opt/curl/include -L/usr/local/opt/curl/lib -lcurl  `pkg-config --cflags --libs opencv4`
Run: 
    ./EmergResponse


