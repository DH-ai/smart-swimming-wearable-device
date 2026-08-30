// swimming_metrics.cpp
//
// Assignment 2, Part 1: Swimming Metrics
//
// Follows the README's "How the calculation works" pipeline exactly:
//   Step 1: time_gap = current_timestamp - previous_timestamp
//   Step 2: integrate acceleration -> speed -> position, for X, Y, AND Z.
//            swimming speed = sqrt(x_speed^2 + y_speed^2)
//   Step 3: a sample is "swimming" when speed >= 0.20 m/s; a stretch where
//            speed stays below that is a rest/turn; each active stretch
//            between rests is one lap.
//
// The file also happens to contain lap_id / is_swimming columns already
// filled in. This version does NOT use them to decide anything -- it
// derives everything itself from raw acceleration, per the README. Those
// two columns are only read in so we can print a side-by-side comparison
// at the end, as a sanity check that our own detection lines up with the
// file's labels.

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iomanip>

using namespace std;

const double ACTIVE_SPEED_THRESHOLD = 0.20; // m/s, per the README

// ---------- Reusable stats helpers ----------

double calculateMean(const vector<double>& v) {
    double sum = 0;
    for (size_t i = 0; i < v.size(); i++) sum += v[i];
    return sum / v.size();
}

double calculateMedian(vector<double> v) { // by value: sorting a copy
    sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2 == 0) return (v[n / 2 - 1] + v[n / 2]) / 2.0;
    return v[n / 2];
}

double calculateStdDev(const vector<double>& v, double mean) {
    double sumSquaredDiff = 0;
    for (size_t i = 0; i < v.size(); i++) {
        double diff = v[i] - mean;
        sumSquaredDiff += diff * diff;
    }
    return sqrt(sumSquaredDiff / v.size());
}

double calculateMax(const vector<double>& v) { return *max_element(v.begin(), v.end()); }
double calculateMin(const vector<double>& v) { return *min_element(v.begin(), v.end()); }

struct Lap {
    int number = 0;
    size_t startIdx = 0, endIdx = 0;
    double time_s = 0;
    double pathLength_m = 0;
    double netDistance_m = 0;
    double avgSpeed_mps = 0;
    double verticalRange_m = 0;
};

int main() {
    vector<double> time, ax, ay, az;
    vector<int> lapIdGiven, isSwimGiven; // read only for the sanity check at the end

    ifstream file("swimming_data.txt");
    if (!file) {
        cout << "Error: Could not open swimming_data.txt!" << endl;
        return 1;
    }

    double t, axi, ayi, azi;
    int lapId, isSwim;
    while (file >> t >> axi >> ayi >> azi >> lapId >> isSwim) {
        time.push_back(t);
        ax.push_back(axi);
        ay.push_back(ayi);
        az.push_back(azi);
        lapIdGiven.push_back(lapId);
        isSwimGiven.push_back(isSwim);
    }
    file.close();

    size_t n = time.size();
    if (n < 2) {
        cout << "Not enough data to analyze." << endl;
        return 1;
    }
    cout << "Loaded " << n << " samples." << endl << endl;

    // ---- Step 1 + Step 2: integrate acceleration -> speed -> position ----
    // Done for X, Y, and Z, exactly as the README describes. Swimming speed
    // then only uses X and Y (Z is depth/vertical bob, not forward travel).
    vector<double> vx(n, 0.0), vy(n, 0.0), vz(n, 0.0);
    vector<double> px(n, 0.0), py(n, 0.0), pz(n, 0.0);
    vector<double> speed(n, 0.0), accelMag(n, 0.0), headingDeg(n, 0.0);

    for (size_t i = 0; i < n; i++) {
        accelMag[i] = sqrt(ax[i] * ax[i] + ay[i] * ay[i] + az[i] * az[i]);

        if (i == 0) {
            vx[i] = vy[i] = vz[i] = 0.0;
            px[i] = py[i] = pz[i] = 0.0;
        } else {
            double time_gap = time[i] - time[i - 1]; // Step 1

            // Step 2
            vx[i] = vx[i - 1] + ax[i] * time_gap;
            vy[i] = vy[i - 1] + ay[i] * time_gap;
            vz[i] = vz[i - 1] + az[i] * time_gap;

            px[i] = px[i - 1] + vx[i] * time_gap;
            py[i] = py[i - 1] + vy[i] * time_gap;
            pz[i] = pz[i - 1] + vz[i] * time_gap;
        }

        speed[i] = sqrt(vx[i] * vx[i] + vy[i] * vy[i]); // X and Y only
        headingDeg[i] = atan2(vx[i], vy[i]) * 180.0 / acos(-1.0);
    }

    // ---- Step 3: find laps from the computed speed, not from the file ----
    vector<int> isSwimComputed(n, 0);
    for (size_t i = 0; i < n; i++)
        isSwimComputed[i] = (speed[i] >= ACTIVE_SPEED_THRESHOLD) ? 1 : 0;

    vector<Lap> laps;
    for (size_t i = 0; i < n; i++) {
        if (isSwimComputed[i] != 1) continue; // resting: skip

        // starting a new active stretch if the previous sample was resting
        // (or this is the very first sample and it's already active)
        bool prevWasResting = (i == 0) || (isSwimComputed[i - 1] == 0);
        if (prevWasResting) {
            Lap lap;
            lap.number = (int)laps.size() + 1;
            lap.startIdx = i;
            laps.push_back(lap);
        }
        laps.back().endIdx = i;
    }

    // per-lap distance / path length / speed / vertical range
    for (auto& lap : laps) {
        double dx = px[lap.endIdx] - px[lap.startIdx];
        double dy = py[lap.endIdx] - py[lap.startIdx];
        lap.netDistance_m = sqrt(dx * dx + dy * dy);
        lap.time_s = time[lap.endIdx] - time[lap.startIdx];

        double vmax = pz[lap.startIdx], vmin = pz[lap.startIdx];
        for (size_t i = lap.startIdx; i <= lap.endIdx; i++) {
            if (i > lap.startIdx) lap.pathLength_m += speed[i] * (time[i] - time[i - 1]);
            vmax = max(vmax, pz[i]);
            vmin = min(vmin, pz[i]);
        }
        lap.verticalRange_m = vmax - vmin;
        lap.avgSpeed_mps = (lap.time_s > 0) ? lap.pathLength_m / lap.time_s : 0.0;
    }

    // ---- Build "swimming only" filtered series for whole-session stats ----
    vector<double> swimSpeed, swimHeading, swimAccelMag, swimLateral;
    for (auto& lap : laps) {
        for (size_t i = lap.startIdx; i <= lap.endIdx; i++) {
            swimSpeed.push_back(speed[i]);
            swimHeading.push_back(headingDeg[i]);
            swimAccelMag.push_back(accelMag[i]);
            swimLateral.push_back(fabs(px[i] - px[lap.startIdx]));
        }
    }

    vector<double> lapTimes, lapAvgSpeeds, straightness;
    double totalNetDistance = 0, totalSwimTime = 0;
    for (auto& lap : laps) {
        lapTimes.push_back(lap.time_s);
        lapAvgSpeeds.push_back(lap.avgSpeed_mps);
        totalNetDistance += lap.netDistance_m;
        totalSwimTime += lap.time_s;
        if (lap.pathLength_m > 0) straightness.push_back(lap.netDistance_m / lap.pathLength_m);
    }
    double totalSessionTime = time.back() - time.front();

    // ============================================================
    cout << fixed << setprecision(3);

    cout << "=== PERFORMANCE ===" << endl;
    cout << "1. Total laps completed:        " << laps.size() << endl;
    cout << "2. Total distance (m):          " << totalNetDistance << endl;
    cout << "3. Total swimming time (s):     " << totalSwimTime << endl;
    cout << "4. Total session time (s):      " << totalSessionTime
         << "  (includes rests)" << endl;
    cout << "5. Average speed (m/s):         " << calculateMean(swimSpeed) << endl;
    cout << "6. Max speed (m/s):             " << calculateMax(swimSpeed) << endl;
    cout << "7. Fastest lap time (s):        " << (lapTimes.empty() ? 0.0 : calculateMin(lapTimes)) << endl;
    cout << endl;

    cout << "=== CONSISTENCY ===" << endl;
    double lapTimeMean = lapTimes.empty() ? 0.0 : calculateMean(lapTimes);
    double lapTimeStdDev = lapTimes.empty() ? 0.0 : calculateStdDev(lapTimes, lapTimeMean);
    cout << "8. Lap time std dev (s):        " << lapTimeStdDev << endl;
    cout << "9. Speed std dev (m/s):         " << calculateStdDev(swimSpeed, calculateMean(swimSpeed)) << endl;
    cout << "10. Lap time consistency (CV %): "
         << (lapTimeMean > 0 ? (lapTimeStdDev / lapTimeMean) * 100.0 : 0.0) << endl;
    cout << endl;

    cout << "=== SWIMMING LINE ===" << endl;
    cout << "11. Average heading (deg):      " << calculateMean(swimHeading) << endl;
    cout << "12. Heading std dev (deg):      " << calculateStdDev(swimHeading, calculateMean(swimHeading)) << endl;
    cout << "13. Avg lateral distance (m):   " << calculateMean(swimLateral) << endl;
    cout << "14. Max lateral distance (m):   " << calculateMax(swimLateral) << endl;
    cout << "15. Straightness ratio (0-1):   " << (straightness.empty() ? 0.0 : calculateMean(straightness)) << endl;
    cout << endl;

    cout << "=== MOTION ===" << endl;
    cout << "16. Avg acceleration mag (m/s^2): " << calculateMean(swimAccelMag) << endl;
    cout << "17. Max acceleration mag (m/s^2): " << calculateMax(swimAccelMag) << endl;
    {
        double vrSum = 0;
        for (auto& lap : laps) vrSum += lap.verticalRange_m;
        cout << "18. Avg vertical oscillation (m): " << (laps.empty() ? 0.0 : vrSum / laps.size()) << endl;
    }
    cout << endl;

    cout << "=== GETTING TIRED ===" << endl;
    if (laps.size() >= 2) {
        double firstSpeed = lapAvgSpeeds.front(), lastSpeed = lapAvgSpeeds.back();
        double speedDecayPct = (firstSpeed > 0) ? ((firstSpeed - lastSpeed) / firstSpeed) * 100.0 : 0.0;
        double firstTime = lapTimes.front(), lastTime = lapTimes.back();
        double timeIncreasePct = (firstTime > 0) ? ((lastTime - firstTime) / firstTime) * 100.0 : 0.0;
        cout << "19. Speed decay, first->last lap (%): " << speedDecayPct << endl;
        cout << "20. Lap time increase, first->last (%): " << timeIncreasePct << endl;
    } else {
        cout << "19. Speed decay, first->last lap (%): N/A (fewer than 2 laps)" << endl;
        cout << "20. Lap time increase, first->last (%): N/A (fewer than 2 laps)" << endl;
    }
    cout << endl;

    cout << "=== PER-LAP BREAKDOWN ===" << endl;
    cout << left << setw(6) << "Lap" << setw(12) << "Time(s)" << setw(14) << "Distance(m)" << setw(14) << "AvgSpeed(m/s)" << endl;
    for (auto& lap : laps) {
        cout << left << setw(6) << lap.number << setw(12) << lap.time_s
             << setw(14) << lap.netDistance_m << setw(14) << lap.avgSpeed_mps << endl;
    }
    cout << endl;

    // ---- Sanity check: does our own detection agree with the file's labels? ----
    int agree = 0;
    for (size_t i = 0; i < n; i++) if (isSwimComputed[i] == isSwimGiven[i]) agree++;
    cout << "=== SELF-CHECK (not one of the 20 metrics) ===" << endl;
    cout << "Our computed is_swimming matches the file's given label on "
         << setprecision(1) << (100.0 * agree / n) << "% of samples." << endl;

    return 0;
}
