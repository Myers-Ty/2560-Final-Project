/**
  ******************************************************************************
  * @file           : main.cpp
  * @brief          : Main program inclduing NortheasternEmergency class
  ******************************************************************************
  *
  ******************************************************************************
  */
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
        /**
         * @brief gives permission to access Google API
         */
        static std::string apiKey;

        /**
         * @brief each index represents repesctive zone with index 0 being zone 1 ... index 3 being zone 4
         */
        int PastEmergencies[4] = {0,0,0,0};

        /**
         * @brief each index represents repesctive zone with index 0 being zone 1 ... index 3 being zone 4
         */
        int OfficersAllocated[4] = {1,1,1,1};

        /**
         * @brief Officer structure that makes up every officer on campus
         * @param ID: officer's badge #
         * @param isAvailable: true = currently at location && false = currently on response to emergency
         */
        struct Officer
        {
            std::string ID;
            bool isAvailable;
        };

        /**
         * @brief Vectors holding officers assigned to each zone.
         * 
         * - zone1_officers: Officers deployed in Zone 1 (Columbus Place AKA Columbus South Sector)
         * - zone2_officers: Officers deployed in Zone 2 (Behrakis AKA West Campus Sector)
         * - zone3_officers: Officers deployed in Zone 3 (Curry Student Center AKA Academics Sector)
         * - zone4_officers: Officers deployed in Zone 4 (Marino AKA East Fenway Sector)
         */
        std::vector<Officer> zone1_officers;
        std::vector<Officer> zone2_officers;
        std::vector<Officer> zone3_officers;
        std::vector<Officer> zone4_officers;

        /**
         * @brief Vector of vectors holding officers assigned to each zone.
         */
        std::vector<std::vector<Officer>> officerZones = {zone1_officers, zone2_officers, zone3_officers, zone4_officers};

       /**
        * @brief Equipment structure that makes up every equipment available on campus
        * @param name: equipments' offical reference name
        * @param weight: how much it weighs limits the 0-1 Knapsack
        * @param importance: determines what choice to make for the 0-1 Knapsack
        */
        struct Equipment
        {
            std::string name;
            int weight;
            int importance;
        };

        /**
         * @brief Emergency map that assigns names and equipment rankings to each emergency
         * @param string: name of emergency type
         * @param vector<Equipment>: equipment relevant to emergency
         */
        std::unordered_map<std::string, std::vector<Equipment>> crimeEquipment =
        {
            {"fire alarm", {{"taser", 2, 0}, {"pepper spray", 1, 0}, {"fire extinguisher", 5, 6}, {"fire axe", 8, 5}}},
            {"fighting", {{"taser", 2, 8}, {"pepper spray", 1, 5}, {"fire extinguisher", 5, 0}, {"fire axe", 8, 0}}}
        };

        /**
         * @brief 0-1 Knapsack problem solver
         * @param items: vector of equipment importances and inventory relevant to emergency
         * @param maxWeight: how much officer can carry based on their strength
         * @retval vector<string>
         */
        std::vector<std::string> solveKnapsack(const std::vector<Equipment> &items, int maxWeight)
        {
            int n = items.size();
            // 2D vector representing maximum importance possible with the first "i" items considering a weight limit of "w"
            std::vector<std::vector<int>> dp(n+1, std::vector<int>(maxWeight+1, 0));

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
            
            // Go through dp table to find the items that were included
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

        /** 
         * @brief Used to encode the overview polyline
         * @param value: raw polyline aqcuired from API
         * @retval string
         */
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
        
        /** 
         *  @brief Used to acquire the overivew polyline
         *  @param origin: where the polyline will start
         *  @param destination: where the polyline will route to
         *  @retval string
         */
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
                // curl code is required and formats the data recieved from Google API into a parsable json
                curl_easy_setopt(curlPoly, CURLOPT_URL, polyUrl.c_str());
                curl_easy_setopt(curlPoly, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlPoly, CURLOPT_WRITEDATA, &readBufferPoly);
                curl_easy_setopt(curlPoly, CURLOPT_FAILONERROR, 1L);
                resPoly = curl_easy_perform(curlPoly);
                curl_easy_cleanup(curlPoly);
                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferPoly);
                if (!jsonResponse["routes"].empty()) {
                    polyline = jsonResponse["routes"][0]["overview_polyline"]["points"].get<std::string>();
                } else {
                    std::cerr << "No routes found." << std::endl;
                }
            }
            return urlEncode(polyline);
        }

        /** 
         *  @brief Used to get the length of polyline
         *  @param origin: where the polyline will start
         *  @param destination: where the polyline will route to
         *  @retval double
         */
        double getPolyLineDistance(std::string origin, std::string destination)
        {
            std::replace(origin.begin(), origin.end(), ' ', '+'); // replace all ' ' to '+'
            std::replace(destination.begin(), destination.end(), ' ', '+'); // replace all ' ' to '+'

            origin = autocompleteAddress(origin);
            destination = autocompleteAddress(destination);
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
                // curl code is required and formats the data recieved from Google API into a parsable json
                curl_easy_setopt(curlPoly, CURLOPT_URL, polyUrl.c_str());
                curl_easy_setopt(curlPoly, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlPoly, CURLOPT_WRITEDATA, &readBufferPoly);
                curl_easy_setopt(curlPoly, CURLOPT_FAILONERROR, 1L);
                resPoly = curl_easy_perform(curlPoly);
                curl_easy_cleanup(curlPoly);

                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferPoly);
                if (!jsonResponse["routes"].empty()) {
                    polyline = jsonResponse["routes"][0]["overview_polyline"]["points"].get<std::string>();
                    distance = jsonResponse["routes"][0]["legs"][0]["distance"]["value"].get<double>();
                } else {
                    std::cerr << "No routes found." << std::endl;
                }
            }
            return distance;
        }

        /** 
         *  @brief Used to select the neareast available officer closest to a emergency based on zone
         *  @param emergencyLocation: where the emergency occurs
         *  @retval None
         */
        void DeployOfficerToIncident(std::string emergencyLocation)
        {
            double shortestPath = std::numeric_limits<double>::max();
            int selectedZone = -1;
            size_t selectedIndex = -1;

            std::vector<std::tuple<int, std::string, double>> deploymentZones;
            std::vector<std::string> zoneNames = {"Columbus Place and Alumni Center", 
                                                "Behrakis Health Sciences Center", 
                                                "Curry Student Center", 
                                                "Marino Recreation Center"};

            for (size_t i = 0; i < zoneNames.size(); ++i)
            {
                double distance = getPolyLineDistance(zoneNames[i], emergencyLocation);
                deploymentZones.push_back(std::make_tuple(i, zoneNames[i], distance));
            }

            std::sort(deploymentZones.begin(), deploymentZones.end(), 
                        [](const std::tuple<int, std::string, double> &a, const std::tuple<int, std::string, double> &b){
                        return std::get<2>(a) < std::get<2>(b);
                    });

            for (const auto &[zoneIndex, zoneName, distance] : deploymentZones)
            {
                for (size_t officerIndex = 0; officerIndex < officerZones[zoneIndex].size(); ++officerIndex)
                {
                    Officer &officer = officerZones[zoneIndex][officerIndex];
                    
                    if (officer.isAvailable)
                    {
                        selectedZone = zoneIndex;
                        selectedIndex = officerIndex;
                        shortestPath = distance;
                        break;
                    }
                }

                if (selectedZone != -1) break;
            }

            if (selectedZone != -1 && selectedIndex != -1)
            {
                Officer &selectedOfficer = officerZones[selectedZone][selectedIndex];
                selectedOfficer.isAvailable = false;

                std::cout << "Deployed officer with badge ID " << selectedOfficer.ID
                        << " from " << zoneNames[selectedZone] << " to " << emergencyLocation
                        << " with distance " << shortestPath << " meters." << std::endl;
            }
            else
            {
                std::cout << "Critical campus-wide emergency present! No available officers to deploy!" << std::endl;
            }
        }
        
        /** 
         *  @brief Used for QOL for user, allowing Google API to guess the address
         *  @param location: incomplete address input by user
         *  @retval string
         */
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
                // Parse the JSON response to extract the polyline
                curl_easy_setopt(curlURL, CURLOPT_URL, autoURL.c_str());
                curl_easy_setopt(curlURL, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curlURL, CURLOPT_WRITEDATA, &readBufferURL);
                curl_easy_setopt(curlURL, CURLOPT_FAILONERROR, 1L);
                resURL = curl_easy_perform(curlURL);
                curl_easy_cleanup(curlURL);
                // Parse the JSON response to extract the polyline
                nlohmann::json jsonResponse = nlohmann::json::parse(readBufferURL);
                if (!jsonResponse["predictions"].empty()) {
                    PlaceID = jsonResponse["predictions"][0]["description"].get<std::string>();
                } else {
                    std::cerr << "No predictions found." << std::endl;
                }
            }
            std::replace(PlaceID.begin(), PlaceID.end(), ' ', '+'); // replace all ' ' to '+'
            return PlaceID;
        }

        /** 
         * @brief Used by curl to help format Google API query
         * @retval size_t
         */
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
            ((std::string*)userp)->append((char*)contents, size * nmemb);
            return size * nmemb;
        }
        
        /**
         * @brief allocates officers to locations across campus based upon past emergency data
         * @param numOfficers: how many officers are currently on duty
         * @retval None 
         */
        void DynamicOfficerAllocation(int numOfficers)
        {
            /* The first location is where the officers are stationed
               The second location is the AED zone the station point is based upon
               Zone 1 ~ Columbus Place (Columbus South Sector)
               Zone 2 ~ Behrakis (West Campus Sector)
               Zone 3 ~ Curry (Academics Sector)
               Zone 4 ~ Marino (East Fenway Sector)
               Zones are stored in the index 1 less than their number
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
                while (i < 4) // loop 4 times once for each zone
                {
                    if (PastEmergencies[i] != 0) // prevent divide by 0
                    {
                        if (OfficersAllocated[i]/PastEmergencies[i] < OfficersAllocated[worstZone]/PastEmergencies[worstZone]) // if ratio worse then allocate officer
                        {
                            worstZone = i;
                        }
                    }
                    i++;
                }
                // allocate officer to zone and remove 1 from officers left to allocate
                OfficersAllocated[worstZone]++;
                numOfficers--;
            }
        }

        /**
         * @brief takes in a CSV file and parses data into usable format by the class
         * @param PathToCSV: string file path to CSV location
         * @retval None
         */
        void ParsePastRecordsCSV(std::string PathToCSV)
        {
            std::ifstream file(PathToCSV); 
            
            if (file.is_open()) {
                std::string parse;

                getline(file, parse); // skips first line of titles

                while (getline(file, parse, ',')) 
                {
                    getline(file, parse);
                    PastEmergencies[stoi(parse)-1]++;
                }
            }
            else
            {
                std::cout << "File failed to open" << std::endl;
            }
        }
        

        /**
        * @brief takes in a CSV file and parses data into usable format by the class
        * @param PathToOfficersCSV: string file path to CSV location
        * @retval None
        */
        void ParseOfficersCSV(std::string PathToOfficersCSV)
        {
            std::ifstream file(PathToOfficersCSV);
            if (file.is_open())
            {
                std::string line;
                std::string ID,deployedZone;
                
                getline(file, line); // skips first line of titles
                while (getline(file, ID, ',') && getline(file, deployedZone))
                {
                    Officer officer;
                    officer.ID = ID;
                    officer.isAvailable = true;

                    switch (stoi(deployedZone))
                    {
                        case 1:
                            zone1_officers.push_back(officer);
                            break;

                        case 2:
                            zone2_officers.push_back(officer);
                            break;

                        case 3:
                            zone3_officers.push_back(officer);
                            break;

                        case 4:
                            zone4_officers.push_back(officer);                    
                            break;
                    }
                    officerZones[0] = zone1_officers;
                    officerZones[1] = zone2_officers;
                    officerZones[2] = zone3_officers;
                    officerZones[3] = zone4_officers;        
                }
            }
            else
            {
                std::cout << "File failed to open" << std::endl;
            }
        }

    public:
        /**
         * @brief Construct a new Northeastern Emergency object
         * @param PathToCSV: string file path to past emergency data
         * @param numOfficers: how many officers are currently on duty
         */
        NortheasternEmergency(std::string PathToCSV, std::string PathToOfficersCSV, int numOfficers)
        {
            ParsePastRecordsCSV(PathToCSV);
            ParseOfficersCSV(PathToOfficersCSV);
            DynamicOfficerAllocation(numOfficers);
        }

        /**
         * @brief chooses the best officer and equipment allocation based upon the emergency
         * @param location: where the emergency is
         * @param emergencyType: type emergency falls under
         * @retval None
         */
        void DeployOfficerAndEquipment(std::string location, std::string emergencyType)
        {
            std::cout<<"Emergency at: "<<location<<std::endl;
            if (DeployOfficerToIncident(location) == 1)
            {

                std::vector<Equipment> equipmentNeeded = crimeEquipment[emergencyType];
                std::vector<std::string> optimalEquipment = solveKnapsack(equipmentNeeded, 15);

                std::cout<<"Optimal equipment that officer should carry for "<<emergencyType<<":"<<std::endl;
                for (const auto &item :optimalEquipment)
                {
                    std::cout<<item<<std::endl;
                }
            }

            else
            {
                std::cout<<"No available officers to deploy!"<<std::endl;
            }
        }

        /**
         * @brief finds the shortest path that uses roads and sidewalks, not just a straight line
         * @param origin: where the officer on call is located
         * @param destination: where the emergency they are responding to is
         * @retval None
         */
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
                // Parse the JSON response to extract the polyline
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

/**
 * @def set to Tyler's API Key
 */
std::string NortheasternEmergency::apiKey = "AIzaSyAVzi2oft7sKVPwi75u-gat3_uk-cwsEB8"; 

/**
  * @brief  The application entry point.
  * @retval int
  */
int main()
{
    // Replace path with path to your .csv file
    int numberOfficers = 25;
    NortheasternEmergency emerg("PastEmergencies.csv", "officers.csv", numberOfficers);

    std::string destination;
    int typeOfEmergency = 1;
    while (true)
    {
        // user interface
        std::cout << "Enter the destination of the emergency: \n";
        getline(std::cin, destination);
        std::cout << "Enter the type of emergency: \n"
                "1. fire alarm \n"
                "2. fighting \n"
                "3. theft \n"
                "4. alcohol overdose \n"
                "5. drug overdose \n"
                "6. acute non lethal injury \n"
                "7. potentially lethal injury \n"
                "8. exit \n";
        std::cin >> typeOfEmergency;
        while(std::cin.fail() || typeOfEmergency < 1 || typeOfEmergency > 8) {
            std::cout << "Enter the type of emergency: \n"
                "1. fire alarm \n"
                "2. fighting \n"
                "3. theft \n"
                "4. alcohol overdose \n"
                "5. drug overdose \n"
                "6. acute non lethal injury \n"
                "7. potentially lethal injury \n"
                "8. exit \n"
                "Error please enter one of the options by their integer number\n";
            std::cin.clear();
            std::cin.ignore(256,'\n');
            std::cin >> typeOfEmergency;
        }
        switch (typeOfEmergency)
        {
            case 1:
                emerg.DeployOfficerAndEquipment(destination, "fire alarm");
                break; 
            case 2:
                emerg.DeployOfficerAndEquipment(destination, "fighting");
                break; 
            case 3:
                emerg.DeployOfficerAndEquipment(destination, "theft");
                break; 
            case 4:
                emerg.DeployOfficerAndEquipment(destination, "alcohol overdose");
                break; 
            case 5:
                emerg.DeployOfficerAndEquipment(destination, "drug overdose");
                break; 
            case 6:
                emerg.DeployOfficerAndEquipment(destination, "acute non lethal injury");
                break; 
            case 7:
                emerg.DeployOfficerAndEquipment(destination, "potentially lethal injury");
                break; 
            case 8:
                return 0;
            default:
                std::cout << "Please enter a valid emergency type. \n";
        }
        std::cin.clear();
        std::cin.ignore(256,'\n');
    }
    return 0;
}