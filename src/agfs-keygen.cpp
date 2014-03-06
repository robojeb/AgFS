#include <iostream>
#include <fstream>
#include <random>
#include <string>

using namespace std;

static const int KEY_LEN = 256;
static const string DIR_PATH = "/var/lib/agfs/";
static const string KEY_LIST_PATH = "/var/lib/agfs/keylist.keys";

int main(int argc, char** argv)
{
  if(argc != 5) {
    cout << "usage: agfs-keygen {host dns name/ip addr} {user} {local mount point} {key file name}" << endl;
  }
  ofstream keyFile;

  //Open the key file
  string keyFileName = DIR_PATH;
  keyFileName.append(argv[4]);
  keyFile.open(keyFileName, ios::out | ios::trunc);

  //Put the hostname/ipaddr into the key file so the client can use it to connect
  keyFile << argv[1] << endl;

  //Open the key list
  ofstream keyList;
  keyList.open(KEY_LIST_PATH, ios::out | ios::app);

  //Prepare the psuedo random number generator
  random_device randDev;
  mt19937 randGen(randDev());
  uniform_int_distribution<> dist(65, 122);
 
  //Generate the key
  string key;
  for(int i = 0; i < KEY_LEN; ++i) {
    key.push_back((char) dist(randGen));
  }
  //Put the key in both the keyfile and the keylist
  keyFile << key << endl;
  keyList << key;

  //Put the user and mount information into the key list
  keyList << "," << argv[2] << "," << argv[3] << endl;

  //Close both files
  keyFile.close();
  keyList.close();

  //Print confirmation message
  cout << "Created key for user " << argv[2] << " mounting to location " << argv[3] << endl;
  cout << "Key: " << endl << key << endl;
 
  return 0;
}
