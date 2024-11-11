#include <iostream>
#include <curl/curl.h> // Used for Map generation : Used Ubuntu to install cURL
#include <fstream>
#include <sstream>
#include <algorithm>
#include <opencv2/core/core.hpp> // Install with Ubuntu
#include <opencv2/highgui/highgui.hpp>  // Install with Ubuntu  
#include <nlohmann/json.hpp> // Install with Ubuntu
#include <vector>
#include <fstream>

class NortheasternEmergency
{
    private:
        static std::string apiKey;
        std::vector<std::string> NUPDLocations;

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
        
        void DynamicOfficerAllocation()
        {

        }

        void ParseCSV(std::string PathToCSV)
        {
            std::ifstream file(PathToCSV); 
            
            if (file.is_open()) {
                std::string Location;
                std::string Emergency;
                while (getline(file, Location, ',')) 
                {
                    std::cout << "Location:" << Location << std::endl ; 

                    getline(file, Emergency);
                    std::cout << "Emergency:" << Emergency << std::endl  ; 

                    // getline(file, Equipment, ','); // can just add rest of getline commands right inside this loop
                }
            }
            else {
                std::cout << "File failed to open" << std::endl;
            }
        }

    public:
    NortheasternEmergency(std::string PathToCSV)
    {
        ParseCSV(PathToCSV);
        DynamicOfficerAllocation();
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
    NortheasternEmergency emerg("PastEmergencies.csv");
    /*
    while (true)
    {
        std::string location;
        std::string location2;
        std::cout << "Enter the origin: ";
        std::getline(std::cin, location);
        std::cout << "Enter the destination: ";
        std::getline(std::cin, location2);
        emerg.ShortestPath(location, location2);
    }
    emerg.ShortestPath("Ell Hall", "Snell Library");
    */
    return 0;
}