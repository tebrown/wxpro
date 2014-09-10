<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
  <html>
  <head>
  <meta http-equiv="refresh" content="10" />
  </head>

  <body>
    <center>
    <table>
    <tr>
      <td align="center" colspan="2">
        <h4>
        <xsl:value-of select="/weather/location/@city"/>
        <xsl:text>, </xsl:text>
        <xsl:value-of select="/weather/location/@state"/><br/>
        <xsl:value-of select="/weather/location/observation/@timestamp"/>
        </h4>
      </td>
    </tr>
    <tr>
     <td>
       <table border="1">
       <tr bgcolor="#9acd32">
         <th align="left">Sensor</th>
         <th align="left">Value</th>
       </tr>
       <xsl:for-each select="weather/location/observation/sensor">
        <xsl:if test=". != ''">
          <tr>
            <td><xsl:value-of select="@type"/></td>
            <td>
               <xsl:value-of select="."/>
               <xsl:text> </xsl:text>
               <xsl:value-of select="@units"/>
            </td>
          </tr>
        </xsl:if>
       </xsl:for-each>
       </table>
      </td>
      <td valign="top">
       <table border="1">
       <tr bgcolor="#ed9a32">
         <th align="left">Trend</th>
         <th align="left">Value</th>
       </tr>
       <xsl:for-each select="weather/location/observation/trend">
       <tr>
         <td><xsl:value-of select="@type"/></td>
         <td>
            <xsl:value-of select="."/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="@units"/>
         </td>
       </tr>
       </xsl:for-each>

       </table>
      </td>
     </tr>
    </table>
  </center>
  </body>
  </html>
</xsl:template>

</xsl:stylesheet>

