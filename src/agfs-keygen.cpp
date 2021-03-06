#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <errno.h>

//Boost includes
#include "constants.hpp"
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;


static const path DIR_PATH{KEY_NAME_PATH};

static const char hexValues[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void usage()
{
  cout << "usage: agfs-keygen {host dns name/ip addr} {port} {user} {local mount point} {key file name}" << endl;
  cout << "  {host dns name/ip addr}" << endl;
  cout << "  Your dns name or ip address." << endl << endl;
  cout << "  {port}" << endl;
  cout << "  The end port clients should use to communicate with you." << endl << endl;
  cout << "  {user}" << endl;
  cout << "  The user account on your system clients with this key will be able to use." << endl << endl;
  cout << "  {local mount point}" << endl;
  cout << "  Where clients will mount your local file system." << endl << endl;
  cout << "  {key file name}" << endl;
  cout << "  The name of the key file on the local machine." << endl << endl << endl;
  cout << "  This program is meant to be run on an AgFS server to generate keys for other users to copy to their machines." << endl;
}

int main(int argc, char** argv)
{
  if(argc != 6) {
    usage();
    return 0;
  }

  // If the key dir doesn't already exist, we attempt to create it.
  if(!exists(DIR_PATH)) {
    if(!create_directory(DIR_PATH)) {
      cerr << "Could not create key directory" << endl;
      return -ENOENT;
    }
  }

  fstream devRandom;
  devRandom.open("/dev/urandom", fstream::in);

  fstream keyListFile;
  keyListFile.open(KEY_LIST_PATH, fstream::out | fstream::app);

  if(!keyListFile.is_open()){
    cerr << "Could not open keyListFile" <<endl;
    return -ENOENT;
  }

  fstream keyFile;
  string keyFileName = KEY_NAME_PATH + argv[5] + KEY_EXTENSION;
  keyFile.open(keyFileName, fstream::out);

  if(!keyFile.is_open()) {
    cerr << "Could not open key file: " << argv[5] << endl;
    return -ENOENT;
  }

  //put the header of the key file
  //Print DNS name and port
  keyFile << argv[1] << endl << argv[2] << endl;

  //Generate the random key
  string keyStr = "";
  size_t count = 0;

  while (count < KEY_LEN) {
    char c;
    devRandom.get(c);
    char lowerByte, upperByte;
    lowerByte = hexValues[0x0f & c];
    upperByte = hexValues[(0xf0 & c) >> 4];
    keyStr += upperByte;
    keyStr += lowerByte;
    ++count;
  }

  //print the key to the keyfile
  keyFile << keyStr << endl;

  //print mount point, uname, key to the key list file
  keyListFile << argv[4] << " " << argv[3] << " " << keyStr << endl;

  //Close files
  devRandom.close();
  keyFile.close();
  keyListFile.close();

  cerr << "Successfuly created new key file" << endl;

  return 0;
}
