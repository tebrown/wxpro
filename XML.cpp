#include "XML.h"
#include <sstream>
#include <iomanip>

using namespace std;

XML::XML( const string &tag ):
   dbg("XML"),
   tag( tag )
{
}

XML::~XML()
{
}

void XML::addNode( const XML &node )
{
   value.clear();
   nodes.push_back( node );
}

string XML::str( int depth, string transform )
{
   std::ostringstream oss;
   if (depth == 0 )
   {
      oss << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl << endl;
      if ( transform.size() > 0 )
      {
         oss << "<?xml-stylesheet type=\"text/xsl\" href=\"" << 
            transform << "\"?>" << endl;
      }
      oss << endl;
   }
   oss << string(depth, ' ' );
   oss << "<" << tag;
   
   // Add attributes
   for ( map<std::string, std::string>::iterator iter = attributes.begin();
         iter != attributes.end();
         ++iter )
   {
      oss << " " << iter->first << "=\"" << iter->second << "\"";
   }
      
   if ( value.size() != 0 )
   {
      oss << ">" << value << "</" << tag << ">" << endl;
   }
   else if ( nodes.size() != 0 )
   {
      oss << ">" << endl;
      for ( vector< XML >::iterator iter = nodes.begin();
            iter != nodes.end();
            ++iter )
      {
         oss << (iter)->str( depth + 1 );
      }
      oss << "</" << tag << ">" << endl;
   }
   else // No value or no subnodes
   {
      oss << "/>" << endl;
   }
   

   return oss.str();
}
