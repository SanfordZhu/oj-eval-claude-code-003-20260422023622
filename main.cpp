#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <unordered_map>

using namespace std;

struct Submission {
    string problem;
    string team;
    string status;
    int time;

    Submission(string p, string t, string s, int time) : problem(p), team(t), status(s), time(time) {}
};

struct ProblemStatus {
    int wrongAttempts = 0;
    bool solved = false;
    int solveTime = 0;
    int frozenSubmissions = 0;
    bool frozen = false;
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    int solvedCount = 0;
    int totalPenalty = 0;
    vector<int> solveTimes;
    bool exists = false;

    Team() {}
    Team(string n) : name(n), exists(true) {}
};

struct Competition {
    bool started = false;
    bool ended = false;
    int duration = 0;
    int problemCount = 0;
    bool frozen = false;
    vector<string> problemNames;
    map<string, Team> teams;
    vector<Submission> submissions;

    void addTeam(const string& teamName) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(teamName) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[teamName] = Team(teamName);
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

        problemNames.clear();
        for (int i = 0; i < probCount; i++) {
            problemNames.push_back(string(1, 'A' + i));
        }
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& teamName, const string& status, int time) {
        if (!started || ended) return;

        submissions.push_back(Submission(problem, teamName, status, time));

        Team& team = teams[teamName];
        ProblemStatus& prob = team.problems[problem];

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
            } else {
                prob.wrongAttempts++;
            }
        }
    }

    vector<pair<Team*, int>> calculateRanking() {
        vector<pair<Team*, int>> ranking;
        for (auto& [name, team] : teams) {
            if (team.exists) {
                ranking.push_back({&team, 0});
            }
        }

        if (!started) {
            sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b) {
                return a.first->name < b.first->name;
            });
        } else {
            sort(ranking.begin(), ranking.end(), [](const auto& a, const auto& b) {
                Team* ta = a.first;
                Team* tb = b.first;

                if (ta->solvedCount != tb->solvedCount) {
                    return ta->solvedCount > tb->solvedCount;
                }

                if (ta->totalPenalty != tb->totalPenalty) {
                    return ta->totalPenalty < tb->totalPenalty;
                }

                int n = min(ta->solveTimes.size(), tb->solveTimes.size());
                for (int i = 0; i < n; i++) {
                    int idx1 = ta->solveTimes.size() - 1 - i;
                    int idx2 = tb->solveTimes.size() - 1 - i;
                    if (ta->solveTimes[idx1] != tb->solveTimes[idx2]) {
                        return ta->solveTimes[idx1] < tb->solveTimes[idx2];
                    }
                }

                return ta->name < tb->name;
            });
        }

        for (int i = 0; i < ranking.size(); i++) {
            ranking[i].second = i + 1;
        }

        return ranking;
    }

    void flush() {
        cout << "[Info]Flush scoreboard.\n";

        auto ranking = calculateRanking();

        for (auto& [team, rank] : ranking) {
            cout << team->name << " " << rank << " " << team->solvedCount << " " << team->totalPenalty;

            for (const string& probName : problemNames) {
                cout << " ";
                auto it = team->problems.find(probName);
                if (it == team->problems.end()) {
                    cout << ".";
                } else {
                    const ProblemStatus& prob = it->second;
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
            auto ranking = calculateRanking();

            bool found = false;
            string lowestTeam;
            int lowestRank = 0;
            string lowestProblem;

            for (int i = ranking.size() - 1; i >= 0; i--) {
                Team* team = ranking[i].first;
                bool hasFrozen = false;
                string firstFrozen;

                for (const string& probName : problemNames) {
                    auto it = team->problems.find(probName);
                    if (it != team->problems.end() && it->second.frozen) {
                        hasFrozen = true;
                        firstFrozen = probName;
                        break;
                    }
                }

                if (hasFrozen) {
                    found = true;
                    lowestTeam = team->name;
                    lowestRank = ranking[i].second;
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

            auto newRanking = calculateRanking();
            int newRank = 0;
            for (auto& [t, r] : newRanking) {
                if (t->name == lowestTeam) {
                    newRank = r;
                    break;
                }
            }

            if (newRank != lowestRank) {
                string replacedTeam;
                for (auto& [t, r] : ranking) {
                    if (r == newRank) {
                        replacedTeam = t->name;
                        break;
                    }
                }

                stringstream ss;
                ss << lowestTeam << " " << replacedTeam << " " << team.solvedCount << " " << team.totalPenalty;
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
        auto it = teams.find(teamName);
        if (it == teams.end() || !it->second.exists) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        auto ranking = calculateRanking();
        for (auto& [team, rank] : ranking) {
            if (team->name == teamName) {
                cout << "[" << teamName << "] NOW AT RANKING " << rank << "\n";
                break;
            }
        }
    }

    void querySubmission(const string& teamName, const string& problem, const string& status) {
        auto it = teams.find(teamName);
        if (it == teams.end() || !it->second.exists) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Submission* lastMatch = nullptr;

        for (auto& sub : submissions) {
            bool match = true;

            if (problem != "ALL" && sub.problem != problem) {
                match = false;
            }

            if (status != "ALL" && sub.status != status) {
                match = false;
            }

            if (sub.team != teamName) {
                match = false;
            }

            if (match) {
                lastMatch = &sub;
            }
        }

        if (!lastMatch) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << teamName << " " << lastMatch->problem << " " << lastMatch->status << " " << lastMatch->time << "\n";
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