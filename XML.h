#ifndef __XML_H__
#define __XML_H__

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iomanip>
#include "Debug.h"

class XML
{
   public:
      XML( const std::string &tag );

      XML( const std::string &_tag, 
           const std::string &attributeKey, 
           const std::string &attributeValue ):
         dbg("XML")
      {
         tag = _tag;
         attributes[attributeKey] = attributeValue;
      }

      template < typename T > XML( const std::string &_tag, T _value, int
            precision = 2 ):
         dbg("XML")
      {
         tag = _tag;
         nodes.clear();
         std::ostringstream oss;
         oss << std::setprecision(precision) << std::fixed << _value;
         value = oss.str();
      }

      template < typename T > XML ( const std::string &_tag, 
                                    const std::string &attributeKey,
                                    const std::string &attributeValue, 
                                    T _value, int precision = 2 )
      {
         nodes.clear();
         std::ostringstream oss;
         oss << std::setprecision(precision) << std::fixed << _value;
         value = oss.str();

         tag = _tag;
         attributes[attributeKey] = attributeValue;
      }

      ~XML();

   public:
      template < typename T > void setValue( T _value, int precision = 2, 
            std::ios_base::fmtflags flags = (std::ios_base::fmtflags)0 )
      {
         nodes.clear();
         std::ostringstream oss;
         oss << std::setprecision(precision) << std::fixed ;
         oss.setf( flags );
         oss << _value;
         value = oss.str();
      }

      void addNode( const XML &node );

      void addAttributes( std::map<std::string, std::string> _attributes )
      {
         attributes.insert(_attributes.begin(), _attributes.end());
      }

      template < typename T > void addAttribute( const std::string &key, 
            T value, 
            int precision = 2 ) 
      {
         std::ostringstream oss;
         oss << std::setprecision(precision) << std::fixed << value;
         attributes[key] = oss.str();
      }

      std::string str( int depth = 0, std::string = "");

   private:
      Debug dbg;
      std::string tag;
      std::map<std::string, std::string> attributes;
      std::vector< XML > nodes;
      std::string value;
};

#endif // __XML_H__
