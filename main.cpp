#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <cstring>

using namespace std;

const int MAX_TEAMS = 10005;
const int MAX_PROBLEMS = 30;

struct ProblemStatus {
    int wrongAttempts = 0;
    bool solved = false;
    int solveTime = 0;
    int frozenSubmissions = 0;
    bool frozen = false;
};

struct Team {
    string name;
    ProblemStatus problems[MAX_PROBLEMS];
    int solvedCount = 0;
    int totalPenalty = 0;
    vector<int> solveTimes;
    bool exists = false;
    int id;

    Team() {}
    Team(const string& n, int i) : name(n), exists(true), id(i) {}

    bool operator<(const Team& other) const {
        if (solvedCount != other.solvedCount) {
            return solvedCount > other.solvedCount;
        }

        if (totalPenalty != other.totalPenalty) {
            return totalPenalty < other.totalPenalty;
        }

        int n = min(solveTimes.size(), other.solveTimes.size());
        for (int i = 0; i < n; i++) {
            int idx1 = solveTimes.size() - 1 - i;
            int idx2 = other.solveTimes.size() - 1 - i;
            if (solveTimes[idx1] != other.solveTimes[idx2]) {
                return solveTimes[idx1] < other.solveTimes[idx2];
            }
        }

        return name < other.name;
    }
};

struct Submission {
    int problem;
    int team;
    string status;
    int time;

    Submission(int p, int t, const string& s, int time) : problem(p), team(t), status(s), time(time) {}
};

struct Competition {
    bool started = false;
    bool ended = false;
    int duration = 0;
    int problemCount = 0;
    bool frozen = false;

    Team teams[MAX_TEAMS];
    vector<Submission> submissions;
    unordered_map<string, int> teamMap;
    int teamCount = 0;

    vector<int> ranking;
    bool rankingValid = false;

    int getTeamId(const string& name) {
        auto it = teamMap.find(name);
        if (it != teamMap.end()) {
            return it->second;
        }
        return -1;
    }

    void addTeam(const string& teamName) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teamMap.find(teamName) != teamMap.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        int id = teamCount++;
        teams[id] = Team(teamName, id);
        teamMap[teamName] = id;
        rankingValid = false;
        cout << "[Info]Add successfully.\n";
    }

    void start(int dur, int probCount) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        started = true;
        duration = dur;
        problemCount = probCount;
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& teamName, const string& status, int time) {
        if (!started || ended) return;

        int teamId = getTeamId(teamName);
        if (teamId == -1) return;

        int probId = problem[0] - 'A';
        if (probId < 0 || probId >= problemCount) return;

        submissions.emplace_back(probId, teamId, status, time);

        Team& team = teams[teamId];
        ProblemStatus& prob = team.problems[probId];

        if (frozen && !prob.solved && !prob.frozen) {
            prob.frozen = true;
        }

        if (prob.frozen) {
            prob.frozenSubmissions++;
        } else if (!prob.solved) {
            if (status == "Accepted") {
                prob.solved = true;
                prob.solveTime = time;
                team.solvedCount++;
                team.totalPenalty += 20 * prob.wrongAttempts + time;
                team.solveTimes.push_back(time);
                rankingValid = false;
            } else {
                prob.wrongAttempts++;
            }
        }
    }

    void calculateRanking() {
        if (rankingValid) return;

        ranking.clear();
        for (int i = 0; i < teamCount; i++) {
            if (teams[i].exists) {
                ranking.push_back(i);
            }
        }

        if (!started) {
            sort(ranking.begin(), ranking.end(), [this](int a, int b) {
                return teams[a].name < teams[b].name;
            });
        } else {
            sort(ranking.begin(), ranking.end(), [this](int a, int b) {
                return teams[a] < teams[b];
            });
        }

        rankingValid = true;
    }

    void flush() {
        cout << "[Info]Flush scoreboard.\n";
        calculateRanking();

        for (int i = 0; i < ranking.size(); i++) {
            Team& team = teams[ranking[i]];
            cout << team.name << " " << (i + 1) << " " << team.solvedCount << " " << team.totalPenalty;

            for (int j = 0; j < problemCount; j++) {
                cout << " ";
                const ProblemStatus& prob = team.problems[j];
                if (prob.frozen) {
                    cout << prob.wrongAttempts << "/" << prob.frozenSubmissions;
                } else if (prob.solved) {
                    if (prob.wrongAttempts == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << prob.wrongAttempts;
                    }
                } else {
                    if (prob.wrongAttempts == 0) {
                        cout << ".";
                    } else {
                        cout << "-" << prob.wrongAttempts;
                    }
                }
            }
            cout << "\n";
        }
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";
        flush();

        vector<string> rankingChanges;

        while (true) {
            calculateRanking();

            bool found = false;
            int lowestTeam = -1;
            int lowestRank = 0;
            int lowestProblem = -1;

            for (int i = ranking.size() - 1; i >= 0; i--) {
                Team& team = teams[ranking[i]];
                bool hasFrozen = false;
                int firstFrozen = -1;

                for (int j = 0; j < problemCount; j++) {
                    if (team.problems[j].frozen) {
                        hasFrozen = true;
                        firstFrozen = j;
                        break;
                    }
                }

                if (hasFrozen) {
                    found = true;
                    lowestTeam = ranking[i];
                    lowestRank = i + 1;
                    lowestProblem = firstFrozen;
                    break;
                }
            }

            if (!found) break;

            Team& team = teams[lowestTeam];
            ProblemStatus& prob = team.problems[lowestProblem];

            int oldSolved = team.solvedCount;
            int oldPenalty = team.totalPenalty;

            prob.frozen = false;

            if (prob.frozenSubmissions > 0) {
                bool foundAccepted = false;
                for (const Submission& sub : submissions) {
                    if (sub.team == lowestTeam && sub.problem == lowestProblem &&
                        sub.status == "Accepted") {
                        foundAccepted = true;
                        prob.solved = true;
                        prob.solveTime = sub.time;
                        team.solvedCount++;
                        team.totalPenalty += 20 * prob.wrongAttempts + sub.time;
                        team.solveTimes.push_back(sub.time);
                        break;
                    }
                }

                if (!foundAccepted) {
                    prob.wrongAttempts += prob.frozenSubmissions;
                }
            }

            prob.frozenSubmissions = 0;
            rankingValid = false;

            calculateRanking();
            int newRank = 0;
            for (int i = 0; i < ranking.size(); i++) {
                if (ranking[i] == lowestTeam) {
                    newRank = i + 1;
                    break;
                }
            }

            if (newRank != lowestRank) {
                string replacedTeam;
                for (int i = 0; i < ranking.size(); i++) {
                    if (i + 1 == newRank) {
                        replacedTeam = teams[ranking[i]].name;
                        break;
                    }
                }

                stringstream ss;
                ss << team.name << " " << replacedTeam << " " << team.solvedCount << " " << team.totalPenalty;
                rankingChanges.push_back(ss.str());
            }
        }

        for (const string& change : rankingChanges) {
            cout << change << "\n";
        }

        flush();
        frozen = false;
    }

    void queryRanking(const string& teamName) {
        int teamId = getTeamId(teamName);
        if (teamId == -1 || !teams[teamId].exists) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        calculateRanking();
        for (int i = 0; i < ranking.size(); i++) {
            if (teams[ranking[i]].name == teamName) {
                cout << "[" << teamName << "] NOW AT RANKING " << (i + 1) << "\n";
                break;
            }
        }
    }

    void querySubmission(const string& teamName, const string& problem, const string& status) {
        int teamId = getTeamId(teamName);
        if (teamId == -1 || !teams[teamId].exists) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        const Submission* lastMatch = nullptr;
        int probFilter = -1;
        if (problem != "ALL") {
            probFilter = problem[0] - 'A';
        }

        for (const Submission& sub : submissions) {
            bool match = true;

            if (sub.team != teamId) {
                match = false;
            }

            if (probFilter != -1 && sub.problem != probFilter) {
                match = false;
            }

            if (status != "ALL" && sub.status != status) {
                match = false;
            }

            if (match) {
                lastMatch = &sub;
            }
        }

        if (!lastMatch) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << teamName << " " << char('A' + lastMatch->problem) << " " << lastMatch->status << " " << lastMatch->time << "\n";
        }
    }

    void end() {
        ended = true;
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Competition comp;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "ADDTEAM") {
            string teamName;
            getline(ss, teamName);
            if (!teamName.empty() && teamName[0] == ' ') {
                teamName = teamName.substr(1);
            }
            comp.addTeam(teamName);
        } else if (cmd == "START") {
            string dummy;
            int duration, problemCount;
            ss >> dummy >> duration >> dummy >> problemCount;
            comp.start(duration, problemCount);
        } else if (cmd == "SUBMIT") {
            string problem, dummy1, teamName, dummy2, status, dummy3;
            int time;
            ss >> problem >> dummy1 >> teamName >> dummy2 >> status >> dummy3 >> time;
            comp.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            comp.flush();
        } else if (cmd == "FREEZE") {
            comp.freeze();
        } else if (cmd == "SCROLL") {
            comp.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string teamName;
            getline(ss, teamName);
            if (!teamName.empty() && teamName[0] == ' ') {
                teamName = teamName.substr(1);
            }
            comp.queryRanking(teamName);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, rest;
            getline(ss, teamName, ' ');
            getline(ss, rest);

            size_t probPos = rest.find("PROBLEM=");
            size_t statusPos = rest.find("STATUS=");

            string problem = "ALL";
            string status = "ALL";

            if (probPos != string::npos) {
                size_t probStart = probPos + 8;
                size_t probEnd = rest.find(" ", probStart);
                if (probEnd == string::npos) probEnd = rest.length();
                problem = rest.substr(probStart, probEnd - probStart);
            }

            if (statusPos != string::npos) {
                size_t statusStart = statusPos + 7;
                size_t statusEnd = rest.find(" ", statusStart);
                if (statusEnd == string::npos) statusEnd = rest.length();
                status = rest.substr(statusStart, statusEnd - statusStart);
            }

            comp.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            comp.end();
            break;
        }
    }

    return 0;
}