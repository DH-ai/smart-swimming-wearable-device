// Goal: read sensor timestamps and hand velocities from a text file,
// separate the data into Air Phase (outside water) and Water Phase
// (inside water), and compute statistical metrics to detect water entry.
 
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
 
using namespace std;
 
// ---- Helper Functions (Part 2) ----
 
// Calculates Mean (Average)
double calculateMean(const vector<double>& v) {
    double sum = 0;
    for (int i = 0; i < v.size(); i++) {
        sum += v[i];
    }
    return sum / v.size();
}
 
// Calculates Median (takes a copy so the original stays in time order)
double calculateMedian(vector<double> v) {
    sort(v.begin(), v.end());
    int n = v.size();
    if (n % 2 == 0) {
        // even count: average the two middle values
        return (v[n / 2 - 1] + v[n / 2]) / 2.0;
    } else {
        // odd count: take the exact middle value
        return v[n / 2];
    }
}
 
// Calculates Standard Deviation
double calculateStdDev(const vector<double>& v, double mean) {
    double sumSquaredDiff = 0;
    for (int i = 0; i < v.size(); i++) {
        double diff = v[i] - mean;
        sumSquaredDiff += diff * diff;
    }
    return sqrt(sumSquaredDiff / v.size());
}
 
// Calculates Average Step Change (how sharply velocity jumps between
// consecutive readings)
double calculateAvgStepChange(const vector<double>& v) {
    double totalStepChange = 0;
    for (int i = 0; i < (int)v.size() - 1; i++) {
        totalStepChange += abs(v[i + 1] - v[i]);
    }
    return totalStepChange / (v.size() - 1);
}
 
int main() {
    vector<double> time_air, vel_air;     // Air Phase data
    vector<double> time_water, vel_water; // Water Phase data
 
    ifstream file("C:\\Users\\Sanaya\\Downloads\\smart-swimming-wearable-device-my-first-branch\\smart-swimming-wearable-device-my-first-branch\\swimming_data.txt"); // file must sit next to the program
 
    if (!file) {
        cout << "Error: Could not open file!" << endl;
        return 1;
    }
 
    double t, v;
    int phase;
 
    // Each row is: timestamp  velocity  phase   (phase: 0 = air, 1 = water)
    // The file already tells us the phase, so we trust that label directly
    // instead of guessing it from the velocity values.
    while (file >> t >> v >> phase) {
        if (phase == 0) { // 0 means Outside Water (Air)
            time_air.push_back(t);
            vel_air.push_back(v);
        } else { // 1 means Inside Water
            time_water.push_back(t);
            vel_water.push_back(v);
        }
    }
    file.close();
 
    cout << "Loaded " << vel_air.size() << " air data points." << endl;
    cout << "Loaded " << vel_water.size() << " water data points." << endl;
    cout << endl;
 
    // ---- Air Phase statistics ----
    double mean_air = calculateMean(vel_air);
    double median_air = calculateMedian(vel_air);
    double stddev_air = calculateStdDev(vel_air, mean_air);
    double stepchange_air = calculateAvgStepChange(vel_air);
 
    // ---- Water Phase statistics ----
    double mean_water = calculateMean(vel_water);
    double median_water = calculateMedian(vel_water);
    double stddev_water = calculateStdDev(vel_water, mean_water);
    double stepchange_water = calculateAvgStepChange(vel_water);
 
    // ---- Comparison: this is the "water entry" signature ----
    double velocity_difference = mean_air - mean_water;
 
    cout << "=== AIR PHASE ===" << endl;
    cout << "Mean Velocity:        " << mean_air << " m/s" << endl;
    cout << "Median Velocity:      " << median_air << " m/s" << endl;
    cout << "Standard Deviation:   " << stddev_air << endl;
    cout << "Average Step Change:  " << stepchange_air << " m/s per reading" << endl;
    cout << endl;
 
    cout << "=== WATER PHASE ===" << endl;
    cout << "Mean Velocity:        " << mean_water << " m/s" << endl;
    cout << "Median Velocity:      " << median_water << " m/s" << endl;
    cout << "Standard Deviation:   " << stddev_water << endl;
    cout << "Average Step Change:  " << stepchange_water << " m/s per reading" << endl;
    cout << endl;
 
    cout << "=== WATER ENTRY DETECTION ===" << endl;
    cout << "Velocity Difference (Air mean - Water mean): "
         << velocity_difference << " m/s" << endl;
    if (velocity_difference > 0) {
        cout << "Water entry causes the hand to decelerate by about "
             << velocity_difference << " m/s on average." << endl;
    }
 
    return 0;
}