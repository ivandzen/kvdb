import os
import subprocess
import random

commandTmp = './build/client/kvdb_cli --host={hostname} --port={port} {query}' 

class Client:
   def __init__(self, hostname, port):
      self.hostname = hostname
      self.port = port
      
   def Insert(self, key, value):
      query = f'INSERT "{key}" "{value}"'
      cmd = commandTmp.format(hostname = self.hostname,
                              port = self.port,
                              query = query)
      code = os.system(cmd)
      return code == 0
      
   def Update(self, key, value):
      query = f'UPDATE "{key}" "{value}"'
      cmd = commandTmp.format(hostname = self.hostname,
                              port = self.port,
                              query = query)
      code = os.system(cmd)
      return code == 0
      
   def Get(self, key):
      query = f'GET "{key}"'
      cmd = commandTmp.format(hostname = self.hostname,
                              port = self.port,
                              query = query)
      return subprocess.check_output(cmd, shell = True)
      
   def Delete(self, key):
      query = f'DELETE "{key}"'
      cmd = commandTmp.format(hostname = self.hostname,
                              port = self.port,
                              query = query)
      code = os.system(cmd)
      return code == 0
      
symbols = "abcdefghijklmnopqrstuvwxyz1234567890 \t\n_"
      
def generateRandomString(from_size, to_size):
   size = int(random.uniform(from_size, to_size))
   result = ""
   for i in range(size):
      result += symbols[int(random.uniform(0, len(symbols) - 1))]
   return result

if __name__ == "__main__":
   client = Client('localhost', 5001)
   numRecords = 5000
   maxKeyLength = 1024
   maxValueLength = 1024 * 1024
   
   for i in range(numRecords):
      key = generateRandomString(500, 1024)
      value = generateRandomString(1000, 10240)
      if not client.Insert(key, value):
         break
