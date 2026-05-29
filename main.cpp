#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct TestCase {
    vector<pair<int, int>> wallet;
    int amount = 0;
    string strategy;
};

struct JsonParser {
    string s;
    size_t pos = 0;

    explicit JsonParser(const string& data) : s(data) {}

    void skipWs() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t')) {
            ++pos;
        }
    }

    bool consume(char c) {
        skipWs();
        if (pos < s.size() && s[pos] == c) {
            ++pos;
            return true;
        }
        return false;
    }

    string parseString() {
        skipWs();
        if (pos >= s.size() || s[pos] != '"') {
            return "";
        }
        ++pos;
        string out;
        while (pos < s.size() && s[pos] != '"') {
            out.push_back(s[pos++]);
        }
        if (pos < s.size()) {
            ++pos;
        }
        return out;
    }

    int parseInt() {
        skipWs();
        int sign = 1;
        if (pos < s.size() && s[pos] == '-') {
            sign = -1;
            ++pos;
        }
        long long value = 0;
        while (pos < s.size() && isdigit(static_cast<unsigned char>(s[pos]))) {
            value = value * 10 + (s[pos++] - '0');
        }
        return static_cast<int>(value * sign);
    }

    vector<pair<int, int>> parsePairArray() {
        vector<pair<int, int>> arr;
        if (!consume('[')) {
            return arr;
        }
        skipWs();
        if (consume(']')) {
            return arr;
        }
        while (true) {
            consume('[');
            int a = parseInt();
            consume(',');
            int b = parseInt();
            consume(']');
            arr.push_back({a, b});
            skipWs();
            if (consume(']')) {
                break;
            }
            consume(',');
        }
        return arr;
    }

    TestCase parseObject() {
        TestCase tc;
        if (!consume('{')) {
            return tc;
        }
        while (true) {
            string key = parseString();
            consume(':');
            if (key == "wallet") {
                tc.wallet = parsePairArray();
            } else if (key == "amount") {
                tc.amount = parseInt();
            } else if (key == "strategy") {
                tc.strategy = parseString();
            } else if (key == "dispense") {
                parsePairArray();
            } else {
                skipWs();
                if (pos < s.size() && s[pos] == '"') {
                    parseString();
                } else if (pos < s.size() && s[pos] == '[') {
                    parsePairArray();
                } else {
                    parseInt();
                }
            }
            skipWs();
            if (consume('}')) {
                break;
            }
            consume(',');
        }
        return tc;
    }

    vector<TestCase> parseArray() {
        vector<TestCase> cases;
        if (!consume('[')) {
            return cases;
        }
        skipWs();
        if (consume(']')) {
            return cases;
        }
        while (true) {
            cases.push_back(parseObject());
            skipWs();
            if (consume(']')) {
                break;
            }
            consume(',');
        }
        return cases;
    }
};

string readFile(const string& path) {
    ifstream in(path);
    if (!in) {
        return "";
    }
    stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

bool canMakeAmount(int target, const vector<pair<int, int>>& denoms, vector<int>& counts) {
    if (target == 0) {
        counts.assign(denoms.size(), 0);
        return true;
    }
    if (target < 0 || denoms.empty()) {
        return false;
    }

    const size_t n = denoms.size();
    vector<char> dp(target + 1, 0);
    vector<vector<int>> prev(target + 1, vector<int>(n, -1));
    dp[0] = 1;

    for (size_t i = 0; i < n; ++i) {
        int value = denoms[i].first;
        int limit = denoms[i].second;
        if (value <= 0 || limit <= 0) {
            continue;
        }
        for (int sum = target; sum >= value; --sum) {
            int maxK = min(limit, sum / value);
            for (int k = 1; k <= maxK; ++k) {
                int before = sum - k * value;
                if (dp[before] && !dp[sum]) {
                    dp[sum] = 1;
                    prev[sum] = prev[before];
                    prev[sum][i] = k;
                    break;
                }
            }
        }
    }

    if (!dp[target]) {
        return false;
    }
    counts.assign(n, 0);
    for (size_t i = 0; i < n; ++i) {
        counts[i] = max(0, prev[target][i]);
    }
    return true;
}

vector<pair<int, int>> buildDispense(const vector<pair<int, int>>& wallet, const vector<int>& counts) {
    vector<pair<int, int>> dispense;
    for (size_t i = 0; i < wallet.size(); ++i) {
        if (counts[i] > 0) {
            dispense.push_back({wallet[i].first, counts[i]});
        }
    }
    return dispense;
}

vector<pair<int, int>> solveMax(vector<pair<int, int>> wallet, int amount) {
    sort(wallet.begin(), wallet.end());
    const size_t highest = wallet.size() - 1;
    const int denom = wallet[highest].first;
    const int available = wallet[highest].second;

    vector<pair<int, int>> rest(wallet.begin(), wallet.end() - 1);
    const int maxUse = min(available, amount / denom);

    for (int use = maxUse; use >= 0; --use) {
        const int remaining = amount - use * denom;
        vector<int> restCounts;
        if (canMakeAmount(remaining, rest, restCounts)) {
            vector<int> full(wallet.size(), 0);
            full[highest] = use;
            for (size_t i = 0; i < rest.size(); ++i) {
                full[i] = restCounts[i];
            }
            return buildDispense(wallet, full);
        }
    }
    return {};
}

vector<pair<int, int>> solveMin(vector<pair<int, int>> wallet, int amount) {
    sort(wallet.begin(), wallet.end());
    const int denom = wallet[0].first;
    const int available = wallet[0].second;

    vector<pair<int, int>> rest(wallet.begin() + 1, wallet.end());
    const int maxUse = min(available, amount / denom);

    for (int use = maxUse; use >= 0; --use) {
        const int remaining = amount - use * denom;
        vector<int> restCounts;
        if (canMakeAmount(remaining, rest, restCounts)) {
            vector<int> full(wallet.size(), 0);
            full[0] = use;
            for (size_t i = 0; i < rest.size(); ++i) {
                full[i + 1] = restCounts[i];
            }
            return buildDispense(wallet, full);
        }
    }
    return {};
}

bool makeWithLimits(int target, const vector<pair<int, int>>& wallet, const vector<int>& low, const vector<int>& high,
                    vector<int>& out) {
    const size_t n = wallet.size();
    vector<char> dp(target + 1, 0);
    vector<vector<int>> prev(target + 1, vector<int>(n, -1));
    dp[0] = 1;

    for (size_t i = 0; i < n; ++i) {
        const int value = wallet[i].first;
        if (value <= 0) {
            continue;
        }
        for (int sum = target; sum >= 0; --sum) {
            if (!dp[sum]) {
                continue;
            }
            for (int k = low[i]; k <= high[i]; ++k) {
                const int next = sum + k * value;
                if (next <= target && !dp[next]) {
                    dp[next] = 1;
                    prev[next] = prev[sum];
                    prev[next][i] = k;
                }
            }
        }
    }

    if (!dp[target]) {
        return false;
    }

    out.assign(n, 0);
    for (size_t i = 0; i < n; ++i) {
        out[i] = max(0, prev[target][i]);
    }
    return true;
}

int spreadOf(const vector<int>& counts) {
    int mn = counts[0];
    int mx = counts[0];
    for (int c : counts) {
        mn = min(mn, c);
        mx = max(mx, c);
    }
    return mx - mn;
}

vector<pair<int, int>> solveUniform(vector<pair<int, int>> wallet, int amount) {
    sort(wallet.begin(), wallet.end());
    const size_t n = wallet.size();
    int maxAvail = 0;
    for (const auto& p : wallet) {
        maxAvail = max(maxAvail, p.second);
    }

    for (int spread = 0; spread <= maxAvail; ++spread) {
        for (int low = 0; low <= maxAvail; ++low) {
            const int high = low + spread;
            vector<int> lowLimits(n);
            vector<int> highLimits(n);
            bool valid = true;

            for (size_t i = 0; i < n; ++i) {
                if (low == 0) {
                    lowLimits[i] = 0;
                    highLimits[i] = min(wallet[i].second, high);
                } else {
                    if (low > wallet[i].second) {
                        valid = false;
                        break;
                    }
                    lowLimits[i] = low;
                    highLimits[i] = min(wallet[i].second, high);
                }
            }
            if (!valid) {
                continue;
            }

            vector<int> counts;
            if (!makeWithLimits(amount, wallet, lowLimits, highLimits, counts)) {
                continue;
            }
            if (spreadOf(counts) <= spread) {
                return buildDispense(wallet, counts);
            }
        }
    }
    return {};
}

vector<pair<int, int>> solve(const TestCase& tc) {
    vector<pair<int, int>> wallet = tc.wallet;
    if (tc.strategy == "MAX") {
        return solveMax(wallet, tc.amount);
    }
    if (tc.strategy == "MIN") {
        return solveMin(wallet, tc.amount);
    }
    if (tc.strategy == "UNIFORM") {
        return solveUniform(wallet, tc.amount);
    }
    return {};
}

string writePairArray(const vector<pair<int, int>>& arr) {
    ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << "[" << arr[i].first << ", " << arr[i].second << "]";
    }
    oss << "]";
    return oss.str();
}

string writeOutput(const vector<vector<pair<int, int>>>& results) {
    ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < results.size(); ++i) {
        if (i > 0) {
            oss << ",\n";
        }
        oss << "{\n\"dispense\": " << writePairArray(results[i]) << "\n}";
    }
    oss << "\n]";
    return oss.str();
}

int main() {
    const string input = readFile("input.json");
    if (input.empty()) {
        cerr << "Cannot read input.json\n";
        return 1;
    }

    JsonParser parser(input);
    const vector<TestCase> cases = parser.parseArray();
    vector<vector<pair<int, int>>> results;
    results.reserve(cases.size());

    for (const TestCase& tc : cases) {
        results.push_back(solve(tc));
    }

    ofstream out("output.json");
    if (!out) {
        cerr << "Cannot write output.json\n";
        return 1;
    }
    out << writeOutput(results);
    return 0;
}
