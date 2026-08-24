add_library(xrXMLParser STATIC)

target_include_directories(xrXMLParser PUBLIC .)

target_sources_grouped(
  TARGET xrXMLParser
  NAME "XMLParser"
  FILES
  StdAfx.cpp
  tinyxml.cpp
  tinyxmlerror.cpp
  tinyxmlparser.cpp
  xrXMLParser.cpp
)

target_link_libraries(xrXMLParser PUBLIC xrCore)
