import httplib, socket, time
host = "localhost"
port = 61000
path = "/"

host = host.replace("http://", "", 1 )
print host

try:
   conn = httplib.HTTPConnection(host, port)
except socket.error as ex:
   print ex
   

while ( 1 ):
   try:
      print "Fetching: %s" % path 
      conn.request("GET", path )
   except socket.gaierror as ex:
      foo = "%s" % ex
      print "Error: %s" % (foo)
      break
   except socket.error as ex:
      print "%s:%d: %s" % (path, port, ex)
   except httplib.CannotSendRequest:
      conn.close()
      conn = httplib.HTTPConnection(host, port)
   else:
      r1 = conn.getresponse()
      print r1.status, r1.reason
      data1 = r1.read()
      print data1

   time.sleep(2)

conn.close()
