#include <iostream>
#include <curl/curl.h> // Used for Map generation : Used Ubuntu to install cURL
#include <fstream>
#include <sstream>
#include <algorithm>
#include <opencv4/opencv2/core/core.hpp> // Install with Ubuntu
#include <opencv2/highgui/highgui.hpp>  // Install with Ubuntu  
#include <nlohmann/json.hpp> // Install with Ubuntu
#include <vector>
#include <unordered_map>
#include <fstream>

class NortheasternEmergency
{
    private:
        static std::string apiKey;
        std::vector<std::string> NUPDLocations;
        int PastEmergencies[4] = {0,0,0,0}; // each index represents repesctive zone with index 0 being zone 1 ... index 4 being zone 5
        int OfficersAllocated[5] = {1,1,1,1,1}; // each index represents repesctive zone with index 0 being zone 1 ... index 4 being zone 5

        // Officer structure
        struct Officer
        {
            std::string ID;
            std::string location;
            bool isAvailable;
        };
        std::vector<Officer> officers;

        // Equipment structure
        struct Equipment
        {
            std::string name;
            int weight;
            int importance;
        };

        // Unordered map of emergency and requisite equipment
        std::unordered_map<std::string, std::vector<Equipment>> crimeEquipment =
        {
            {"fire alarm", {{"taser", 2, 0}, {"pepper spray", 1, 0}, {"fire extinguisher", 5, 6}, {"fire axe", 8, 5}}},
            {"fighting", {{"taser", 2, 8}, {"pepper spray", 1, 5}, {"fire extinguisher", 5, 0}, {"fire axe", 8, 0}}}
        };

        // method to solve 01 knapsack problem and choose optimal eq.
        std::vector<std::string> solveKnapsack(const std::vector<Equipment> &items, int maxWeight)
        {
            int n = items.size();
            std::vector<std::vector<int>> dp(n+1, std::vector<int>(maxWeight+1, 0));// 2D vector -rprsnting. max. import. posbl. w/ first i items w/ weight lim. of w

            for (int i = 1; i <= n; ++i) //Iterate over each item
            {
                for (int w = 1; w <= maxWeight; ++w) // Iterate over each weight limit
                {
                    if (items[i-1].weight <= maxWeight) // If item can fit in the knapsack with the current weight limit
                    {
                        dp[i][w] = std::max(dp[i-1][w], dp[i-1][w-items[i-1].weight]+items[i-1].importance); // Choose the maximum
                    }

                    else // If not, do not include
                    {
                        dp[i][w] = dp[i-1][w];
                    }
                }
            }
            // Go back to find chosen items
            std::vector<std::string> chosenItems;
            int w = maxWeight;
            
            // Go through dp table to find the items taht were included
            for (int i = n; i>0 && w > 0; --i)
            {
                if (dp[i][w] != dp[i-1][w]) // If current != previous the item was included
                {
                    chosenItems.push_back(items[i-1].name); // Add item to list
                    w -= items[i-1].weight; // Reduce remaining allowed weight
                }
            }
            return chosenItems;
        }

        // Used to encode the overivew polyline
        std::string urlEncode(const std::string &value) 
        {
            std::ostringstream escaped;
            escaped.fill('0');
            escaped << std::hex;
            
            for (const auto &c : value) {
                // Keep alphanumeric and few other characters intact
                if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
                    escaped << c;
                } else {
                    // Encode other characters
                    escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
                }
            }
            return escaped.str();
        }
        
        // Used to acquire the overivew polyline
        std::string getPolyLine(std::string origin, std::string destination)
        {
            // Construct the URL for the Directions API
            std::string polyUrl = "https://maps.googleapis.com/maps/api/directions/json?"
                            "origin=" + origin +
                            "&destination=" + destination + 
                            "&mode=walking"
                            "&location=Northeastern+University" // limit search results to northeastern area
                            "&radius=3000" // Limit search radius to within 3km of Northeastern
                            "&key=" + apiKey;

            std::string polyline;
            
            std::string readBufferPoly;
            CURL* curlPoly;
            CURLcode resPoly;
            curlPoly = curl_easy_init();
            
            if (curlPoly) {
                curl_easy_setopt(curlPoly, CURLOPT_URL, polyUrl.c_str());
                curl_easy_setopt(curlPoly, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlPoly, CURLOPT_WRITEDATA, &readBufferPoly);
                curl_easy_setopt(curlPoly, CURLOPT_FAILONERROR, 1L);
                resPoly = curl_easy_perform(curlPoly);
                curl_easy_cleanup(curlPoly);
                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferPoly);
                // std::cout << readBufferPoly << std::endl; // Used for DEBUGGING
                if (!jsonResponse["routes"].empty()) {
                    polyline = jsonResponse["routes"][0]["overview_polyline"]["points"].get<std::string>();
                    std::cout << "polyline found" << std::endl; // Used for DEBUGGING
                } else {
                    std::cerr << "No routes found." << std::endl;
                }
            }
            return urlEncode(polyline);
        }

        double getPolyLineDistance(std::string origin, std::string destination)
        {
            // Construct the URL for the Directions API
            std::string polyUrl = "https://maps.googleapis.com/maps/api/directions/json?"
                            "origin=" + origin + // Add NEU to address for accuracy
                            "&destination=" + destination + // Add NEU to address for accuracy
                            "&mode=walking"
                            "&location=Northeastern+University" // limit search results to northeastern area
                            "&radius=3000" // Limit search radius to within 3km of Northeastern
                            "&key=" + apiKey;

            std::string polyline;
            
            std::string readBufferPoly;
            CURL* curlPoly;
            CURLcode resPoly;
            curlPoly = curl_easy_init();

            double distance = 0;
            
            if (curlPoly) {
                curl_easy_setopt(curlPoly, CURLOPT_URL, polyUrl.c_str());
                curl_easy_setopt(curlPoly, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlPoly, CURLOPT_WRITEDATA, &readBufferPoly);
                curl_easy_setopt(curlPoly, CURLOPT_FAILONERROR, 1L);
                resPoly = curl_easy_perform(curlPoly);
                curl_easy_cleanup(curlPoly);

                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferPoly);
                std::cout << readBufferPoly << std::endl;
                if (!jsonResponse["routes"].empty()) {
                    polyline = jsonResponse["routes"][0]["overview_polyline"]["points"].get<std::string>();
                    distance = jsonResponse["routes"][0]["legs"][0]["distance"]["value"].get<double>();
                    std::cout << "polyline found" << std::endl; // Used for DEBUGGING
                } else {
                    std::cerr << "No routes found." << std::endl;
                }
            }
            return distance;
        }

        void DeployOfficer(std::string emergencyLocation, std::vector<Officer> officers)
        {
            double shortestPath = std::numeric_limits<double>::max();
            Officer* nearestOfficer = nullptr;

            for (auto& officer : officers)
            {
                if (officer.isAvailable)
                {
                    double pathLength = getPolyLineDistance(officer.location, emergencyLocation);

                    if (pathLength < shortestPath)
                    {
                        shortestPath = pathLength;
                        nearestOfficer = &officer;
                    }
                }
            }

            if (nearestOfficer != nullptr)
            {
                nearestOfficer->isAvailable = false;
                std::cout<<"Deployed Officer ID: "<<nearestOfficer->ID<<" from "<<nearestOfficer->location<<" to "<<emergencyLocation<< " with distance " <<shortestPath<<" meters"<<std::endl;
                ShortestPath(nearestOfficer->location, emergencyLocation);
            }
            else
            {
                std::cout<<"No available officers to deploy!"<<std::endl;
            }
        }  
        std::string autocompleteAddress(std::string location)
        {
            // Construct the URL for the Directions API
            std::string autoURL = "https://maps.googleapis.com/maps/api/place/autocomplete/json"
                                "?input=" + location +
                                "&location=Northeastern+University"
                                "&radius=3000" // Limit search radius to within 3km of Northeastern
                                "&key=" + apiKey;

            std::string PlaceID;

            std::string readBufferURL;
            CURL* curlURL;
            CURLcode resURL;
            curlURL = curl_easy_init();
            if (curlURL) {
                curl_easy_setopt(curlURL, CURLOPT_URL, autoURL.c_str());
                curl_easy_setopt(curlURL, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlURL, CURLOPT_WRITEDATA, &readBufferURL);
                curl_easy_setopt(curlURL, CURLOPT_FAILONERROR, 1L);
                resURL = curl_easy_perform(curlURL);
                curl_easy_cleanup(curlURL);
                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferURL);
                std::cout << readBufferURL << std::endl; // Used for Debugging
                std::cout << readBufferURL << std::endl; // Used for Debugging
                if (!jsonResponse["predictions"].empty()) {
                    PlaceID = jsonResponse["predictions"][0]["description"].get<std::string>();
                } else {
                    std::cerr << "No predictions found." << std::endl;
                }
            }
            std::cout << PlaceID << std::endl; // Used for DEBUGGING
            std::replace(PlaceID.begin(), PlaceID.end(), ' ', '+'); // replace all ' ' to '+'
            return PlaceID;
        }

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }
        
        void DynamicOfficerAllocation(int numOfficers)
        {

            /* The first location is where the officers are stationed
               The second location is the AED zone the station point is based upon
               Zone 1 ~ Columbus Place (Columbus South Sector)
               Zone 2 ~ Behrakis (West Campus Sector)
               Zone 3 ~ Curry (Academics Sector)
               Zone 4 ~ Marino (East Fenway Sector)
            */
            numOfficers = numOfficers - 4; // Fair to assume at least 4 officers are always on duty due to size of our university so one officer is assigned per zone
            while (numOfficers > 0)
            {
                int i = 1;
                int worstZone = 0;
                while (PastEmergencies[worstZone] == 0) // prevent divide by 0
                {
                    worstZone++;
                }
                while (i < 4)
                {
                    if (PastEmergencies[i] != 0) // prevent divide by 0
                    {
                        if (OfficersAllocated[i]/PastEmergencies[i] < OfficersAllocated[worstZone]/PastEmergencies[worstZone])
                        {
                            worstZone = i;
                        }
                    }
                    i++;
                }
                OfficersAllocated[worstZone]++;
                numOfficers--;
            }
        }

        void ParseCSV(std::string PathToCSV)
        {
            std::ifstream file(PathToCSV); 
            
            if (file.is_open()) {
                std::string parse;

                getline(file, parse); // skips first line of titles

                while (getline(file, parse, ',')) 
                {
                    // std::cout << "Location:" << parse << " | "; 

                    getline(file, parse, ',');
                    // std::cout << "Emergency:" << parse << " | "  ;

                    getline(file, parse);
                    PastEmergencies[stoi(parse)-1]++;

                    // getline(file, Equipment, ','); // can just add rest of getline commands right inside this loop
                }
            }
            else
            {
                std::cout << "File failed to open" << std::endl;
            }
        }
        
        void ParseOfficersCSV(std::string PathToOfficersCSV)
        {
            std::ifstream file(PathToOfficersCSV);
            if (file.is_open())
            {
                std::string line;
                
                while (getline(file, line))
                {
                    std::stringstream ss(line);
                    std::string ID, location, isAvailableStr;
                    bool isAvailable;

                    getline(ss, ID, ',');
                    getline(ss, location, ',');
                    getline(ss, isAvailableStr);

                    if (isAvailableStr == "true")
                    {
                        isAvailable = true;
                    }
                    else
                    {
                        isAvailable = false;
                    }

                    Officer officer;
                    officer.ID = ID;
                    officer.location = location;
                    officer.isAvailable = isAvailable;
                    officers.push_back(officer);
                }
                file.close();
            }

            else
            {
                std::cout << "File failed to open" << std::endl;
            }
        }

    public:
    NortheasternEmergency(std::string PathToCSV, int numOfficers)
    {
        ParseCSV(PathToCSV);
        DynamicOfficerAllocation(numOfficers);
    }

    void DeployOfficerAndEquipment(std::string location, std::string emergencyType)
    {
        std::cout<<"Emergency at: "<<location<<std::endl;
        DeployOfficer(location, officers);

        std::vector<Equipment> equipmentNeeded = crimeEquipment[emergencyType];
        std::vector<std::string> optimalEquipment = solveKnapsack(equipmentNeeded, 15);

        std::cout<<"Optimal equipment for "<<emergencyType<<std::endl;
        for (const auto &item :optimalEquipment)
        {
            std::cout<<item<<std::endl;
        }
    }

    void ShortestPath(std::string origin, std::string destination)
    {
        // Format the origin and destination to match Google's API Format
        std::replace(origin.begin(), origin.end(), ' ', '+'); // replace all ' ' to '+'
        std::replace(destination.begin(), destination.end(), ' ', '+'); // replace all ' ' to '+'

        origin = autocompleteAddress(origin);
        destination = autocompleteAddress(destination);
        // std::cout << getPolyLine(origin, destination) << std::endl; // Used for Debugging
        std::string url = "https://maps.googleapis.com/maps/api/staticmap?"
                    "size=600x600"
                    "&markers=color:blue%7Clabel:S%7C" + origin + // Add NEU to address for accuracy
                    "&markers=color:red%7Clabel:D%7C" + destination + // Add NEU to address for accuracy
                    "&path=enc:" + getPolyLine(origin, destination) + // Shows actual path for officers to take
                    "&maptype=satellite" // Change map type to satellite
                    //"&zoom=17" // Adjust zoom level to focus on the path
                    "&key=" + this->apiKey;
        // std::cout << url << std::endl; //Used for DEBUGGING
        CURL* curl;
        CURLcode res;
        std::string readBuffer;
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        // Save the image to a file
        std::ofstream outFile("route_map.png", std::ios::binary);
        outFile.write(readBuffer.c_str(), readBuffer.size());
        outFile.close();
        cv::Mat image= cv::imread("route_map.png");   
        std::string windowName = "Emergency Route"; //Name of the window
        cv::namedWindow(windowName); // Create a window
        cv::imshow(windowName, image); // Show our image inside the created window.
        cv::waitKey(0); // Wait for any keystroke in the window
        cv::destroyWindow(windowName); //destroy the created window
    }

    
   
};

std::string NortheasternEmergency::apiKey = "AIzaSyAVzi2oft7sKVPwi75u-gat3_uk-cwsEB8"; // Default set to Tyler's API Key

int main()
{
    // Replace path with path to your .csv file
    int numberOfficers = 25;
    NortheasternEmergency emerg("PastEmergencies.csv", numberOfficers);

    std::string destination;
    int typeOfEmergency;
    while (true)
    {
        std::cout << "Enter the destination of the emergency: ";
        getline(std::cin, destination);
        std::cout << "Enter the type of emergency: \n"
                    "1. fire alarm \n"
                    "2. fighting \n";
        std::cin >> typeOfEmergency;
        switch (typeOfEmergency)
        {
            case 1:
                emerg.DeployOfficerAndEquipment(destination, "fire alarm");
                break; 
            case 2:
                emerg.DeployOfficerAndEquipment(destination, "fighting");
                break; 
            default:
                std::cout << "Please enter a valid emergency type.";
        }
    }
    return 0;
}