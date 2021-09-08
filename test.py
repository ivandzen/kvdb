import os
import pathlib
import subprocess
import random

commandTmp = '../build/client/kvdb_cli --host={hostname} --port={port} {query}' 

def StartServer():
   os.system("docker-compose up -d server")
   
def StopServer():
   os.system("docker-compose stop")

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
      return subprocess.check_output(cmd, shell = True).decode('utf-8')
      
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

   # make sure to run this test from top-level kvdb directory!
   os.chdir("./docker")
   StartServer()
   
   client = Client('localhost', 5001)
   numberToInsert = 1000
   maxKeyLength = 1024
   maxValueLength = 1024 * 1024
   compareData = {}
   
   insertFailed = 0
   getFailed = 0
   brokenData = 0
   deleteFailed = 0
   
   ######################################################################
   # Inserting records
   for i in range(numberToInsert):
      key = generateRandomString(500, maxKeyLength)
      value = generateRandomString(1024 * 40, 1024 * 50)
      if client.Insert(key, value):
         compareData[key] = value
      else:
         insertFailed += 1
         
   ######################################################################
   # Restarting Server
   StopServer()
   StartServer()
        
   ######################################################################
   # Checking records
   for key, value in compareData.items():
      try:
         valueRemote = client.Get(key)
         if value != valueRemote:
            brokenData += 1
      except e:
         getFailed += 1
         
   ######################################################################
   # Delete records
   for key, value in compareData.items():
      if not client.Delete(key):
         deleteFailed += 1
         
   ######################################################################
   # Print results
   
   print(f"Number to insert : {numberToInsert}")
   print(f"Insert failed : {insertFailed}")
   print(f"Get failed : {getFailed}")
   print(f"Broken data : {brokenData}")
   print(f"Delete failed: {deleteFailed}")
   
   StopServer()
   
   assert(insertFailed == 0)
   assert(getFailed == 0)
   assert(brokenData == 0)
   assert(deleteFailed == 0)
   
   print("ALL TESTS ARE OK")
         
