// Copy the contents of this file into main.cpp and run it to test edge cases.

void unitTest()
{
    // Test data initialization
    std::string PathToCSV = "PastEmergencies.csv";
    std::string PathToOfficersCSV = "officers.csv";
    int numberOfficers = 25;

    // Initialize the NortheasternEmergency object
    NortheasternEmergency emerg(PathToCSV, PathToOfficersCSV, numberOfficers);

    // Test cases: normal cases
    std::vector<std::pair<std::string, std::string>> normalCases =
    {
        {"Curry Student Center", "fire alarm"},
        {"Marino Recreation Center", "fighting"},
        {"Behrakis Health Sciences Center", "theft"},
        {"Columbus Place", "alcohol overdose"},
        {"Snell Library", "drug overdose"},
        {"Krentzman Quad", "acute non lethal injury"},
        {"Ell Hall", "potentially lethal injury"}
    };

    // Edge cases: locations with unusual names, emergencies with low inventory, no available officers, invalid emergency types
    std::vector<std::pair<std::string, std::string>> edgeCases = 
    {
        {"Remote Building X", "fire alarm"}, // Unusual location
        {"1600 Pennsylvania Avenue", "fire alarm"}, // Extremely distant address
        {"Marino Recreation Center", "unknown emergency"}, // Invalid emergency type
        {"Krentzman Quad", "potentially lethal injury"}, // High severity, testing multiple times
        {"Columbus Place", "fire alarm"} // Repeating to exhaust resources
        {"Columbus Place", "fire alarm"}
        {"Columbus Place", "fire alarm"}
    };

    // Iterate through edge cases
    for (const auto& testCase : edgeCases)
    {
        std::string location = testCase.first;
        std::string emergencyType = testCase.second;

        // Deploy officers and equipment
        std::cout << "Testing DeployOfficerAndEquipment for " << emergencyType << " at " << location << "..." << std::endl;
        emerg.DeployOfficerAndEquipment(location, emergencyType);
    }

    // Test invalid scenarios
    std::cout << "Testing DeployOfficerAndEquipment with invalid emergency type..." << std::endl;
    emerg.DeployOfficerAndEquipment("Curry Student Center", "invalid_type");

    // Testing no officers available scenario
    // Deploy all officers
    for (int i = 0; i < 25; ++i)
    {
        emerg.DeployOfficerAndEquipment("Columbus Place", "fire alarm");
    }

    std::cout << "Testing DeployOfficerToIncident with no available officers..." << std::endl;
    emerg.DeployOfficerAndEquipment("Curry Student Center", "fire alarm"); // Should indicate no available officers

    std::cout << "All unit tests completed." << std::endl;
}
