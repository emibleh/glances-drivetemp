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
  {"Temperature_Celsius",         9}, // SATA
  {"Airflow_Temperature_Cel",     9}, // SATA alt
  {"Temperature:",                1}, // NVMe
  {"Current Drive Temperature:",  3},   // SAS/SCSI
};

bool sanityCheck(const string& d) {
  return d.rfind("/dev/", 0) == 0 && d.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/_-") == string::npos;
}

vector<string> split(const string& text){
  vector<string> out;
  istringstream iss(text);
  string s;
  while (getline(iss, s)) out.push_back(s);
  return out;
}

vector<string> splitWords(const string& line){
  istringstream iss(line);
  return {istream_iterator<string>(iss), istream_iterator<string>()};
}

vector<string> getDrives(const string& smartOut){
  vector<string> drives;
  for (const auto& line : split(smartOut)) {
    auto words = splitWords(line);
    if (!words.empty() && sanityCheck(words[0])) drives.push_back(words[0]);
  }
  return drives;
}

bool getTemp(const vector<string>& lines, const vector<Field>& fields, int& out){
  for (const auto& f : fields) {
    for (const auto& line : lines) {
      if (line.find(f.key) == string::npos) continue;

      auto words = splitWords(line);
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
  for (const auto& d : drives) {
    auto lines = split(readCommand("/usr/sbin/smartctl -A " + d));

    Drive drive{d};
    getTemp(lines, tempFields, drive.temp);
    Data.push_back(drive);
  }
  return Data;
}

// config imported from original py script
constexpr uint16_t PORT = 7634;
constexpr const char* BIND = "0.0.0.0";
constexpr auto CACHE_TTL = chrono::seconds(5);
const timeval SEND_TIMEOUT{2, 0};

string buildResponse(){
  string out;
  for (const auto& d : getData(getDrives(readCommand("timeout 10 /usr/sbin/smartctl --scan")))){
    if (d.temp < 0) continue;
    out += "|" + d.dir + "|" + d.model + "|" + to_string(d.temp) + "|C|";
  }
  return out;
}

// also ported from original script
const string& cachedResponse(){
  static string response;
  static chrono::steady_clock::time_point stamp;
  static bool primed = false;
  if (!primed || chrono::steady_clock::now() - stamp > CACHE_TTL){
    response = buildResponse();
    stamp = chrono::steady_clock::now();
    primed = true;
  }
  return response;
}

struct Fd{
  int fd;
  explicit Fd(int f) : fd(f) {}
  ~Fd(){ if (fd >= 0) close(fd); }
  Fd(const Fd&) = delete;
  Fd& operator=(const Fd&) = delete;
};

[[noreturn]] void die(const char* what) { perror(what); exit(1); }

int makeListener(){
  int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
  if (fd < 0) die("socket");

  int on = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, BIND, &addr.sin_addr) != 1) die("bad BIND address");
  if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) die("bind");
  if (listen(fd, 16) < 0) die("listen");
  return fd;
}

void serve(int client, const string& body){
  setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &SEND_TIMEOUT, sizeof SEND_TIMEOUT);

  for (size_t sent = 0; sent < body.size();){
    ssize_t n = send(client, body.data() + sent, body.size() - sent, MSG_NOSIGNAL);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) return; // timout
    sent += n;
  }
  shutdown(client, SHUT_WR);
}

volatile sig_atomic_t quit = 0;

int main(){
  struct sigaction sa{};
  sa.sa_handler = [](int) { quit = 1; };
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  Fd listener{makeListener()};
  pollfd pfd{listener.fd, POLLIN, 0};
  cachedResponse();

  while (!quit){
    if (poll(&pfd, 1, 1000) <= 0) continue;
    Fd client{accept4(listener.fd, nullptr, nullptr, SOCK_CLOEXEC)};
    if (client.fd < 0){
      if (errno == EMFILE || errno == ENFILE || errno == ENOMEM) sleep(1);
      continue;
    }
    serve(client.fd, cachedResponse());
  }
}
