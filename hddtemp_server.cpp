#include "hddtemp_server.h"

using namespace std;

string readCommand(string cmd){
	string data;
	FILE * stream;
	const int max_buffer = 256;
	char buffer[max_buffer];
	cmd.append(" 2>&1");

	stream = popen(cmd.c_str(), "r");
	if (!stream) return data;

	while (!feof(stream))
		if(fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
	pclose(stream);
	
	return data;
}

const vector<Field> tempFields = {
  {"Temperature_Celsius",     9}, // SATA/SAS
  {"Airflow_Temperature_Cel", 9}, // SATA
  {"Temperature:",            1}, // NVMe
};

// The following could be improved, it is quite verbose.
vector<string> getDrives(const string& smartOut){
  vector<string> drives;
  istringstream iss(smartOut);
  string line;
  while (getline(iss, line)) {
    istringstream ls(line);
    string dev;
    if (ls >> dev) drives.push_back(dev);
  }
  return drives;
}

vector<string> splitLines(const string& text){
  vector<string> lines;
  istringstream iss(text);
  string line;
  while (getline(iss, line)) lines.push_back(line);
  return lines;
}

vector<string> splitWords(const string& line){
  vector<string> words;
  istringstream iss(line);
  string w;
  while (iss >> w) words.push_back(w);
  return words;
}

bool getTemp(const vector<string>& lines, const vector<Field>& fields, int& out) {
  for (const auto& f : fields) {
    for (const auto& line : lines) {
      if (line.find(f.key) == string::npos) continue;

      vector<string> words = splitWords(line);
      if ((int)words.size() <= f.tk) continue;

      try {
        out = stoi(words[f.tk]);
        return true;
      } catch (...) {}
    }
  }
  return false;
}

vector<Drive> getData(const vector<string>& drives){
  vector<Drive> Data;
  if (drives.empty()) return Data;

  for (const auto& d : drives) {
    vector<string> lines = splitLines(readCommand("/usr/sbin/smartctl -a " + d));

    Drive drive;
    drive.dir = d;
    getTemp(lines, tempFields, drive.temp);

    Data.push_back(drive);
  }
  return Data;
}

int main() {
  string smartOut = readCommand("/usr/sbin/smartctl --scan");
  vector<string> drives = getDrives(smartOut);
  vector<Drive> data = getData(drives);

  // debug drive output
  for (const auto& d : data) { cout << d.dir + " "<< d.temp << endl; }

  return 0;
}
