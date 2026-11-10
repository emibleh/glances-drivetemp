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

vector<string> getDrives(const string& smartOut) {
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

vector<Drive> getData(const vector<string>& drives){
  vector<Drive> Data;

  if (drives.empty()) return Data;

  for (const auto& d : drives) {
    string str = readCommand("/usr/sbin/smartctl -a " + d);

    // if this doesn't show smartctl -a /dev/... output then something is fucked
    cout << str << endl;


    Data.push_back({d, 0});
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
