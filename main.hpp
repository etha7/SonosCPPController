//Header file for main.cpp

#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string.h>
#include <boost/regex.hpp>
#include <boost/asio.hpp>

#define  URL_AV "/MediaRenderer/AVTransport/Control"
#define  BODY_PLAY  " <s:Envelope "\
       "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "\
       "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"\
       "<s:Body>"\
         "<u:Play "\
             "xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"\
             "<InstanceID>0</InstanceID>"\
             "<Speed>1</Speed>"\
             "</u:Play>"\
      "</s:Body>"\
   "</s:Envelope>"

#define  BODY_PAUSE  " <s:Envelope "\
       "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "\
       "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"\
       "<s:Body>"\
         "<u:Pause "\
             "xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"\
             "<InstanceID>0</InstanceID>"\
             "<Speed>1</Speed>"\
             "</u:Pause>"\
      "</s:Body>"\
   "</s:Envelope>"

#define  BODY_PREVIOUS  " <s:Envelope "\
       "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "\
       "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"\
       "<s:Body>"\
         "<u:Previous "\
             "xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"\
             "<InstanceID>0</InstanceID>"\
             "</u:Previous>"\
      "</s:Body>"\
   "</s:Envelope>"

#define  BODY_NEXT  " <s:Envelope "\
       "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "\
       "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"\
       "<s:Body>"\
         "<u:Next "\
             "xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">"\
             "<InstanceID>0</InstanceID>"\
             "</u:Next>"\
      "</s:Body>"\
   "</s:Envelope>"


//Utility functions
    
        std::string getXmlByTag(std::string, std::string);
        std::vector<std::string> * getXmlByTags(std::string, std::vector<std::string>);

        //Get the data of a given XML tag
        std::string getXmlByTag(std::string key, std::string xml)
        {
           boost::regex e(".*<"+key+">(.*)<\/"+key+">.*");
           boost::smatch matches;
           if(!boost::regex_match(xml,matches,e))
                return "";
           else
                return matches[1]; 
        }
        
        //Get the data of a list of given XML tags
        std::vector<std::string>* getXmlByTags(std::vector<std::string>* keys, std::string xml){
    
           std::vector<std::string>* values = new std::vector<std::string>();
           std::string temp;
           for(std::vector<std::string>::iterator it = keys->begin(); it != keys->end();++it){
               if((temp=getXmlByTag(*it,xml)).empty())
               {
                   //Missing one or more keys, return an empty vector
                   return new std::vector<std::string>();
               }
               else
               {
                   values->push_back(getXmlByTag(*it, xml));
               }
           }   
           return values; 
        }



//Class to handle HTTP messages. Underlying HTTP service: libcurl. 
class HttpMessenger {

       CURL * easyhandle;
       public:
            HttpMessenger()
            {
                curl_global_init(CURL_GLOBAL_ALL);  //Should only be called once per program. Add to app initialization 
                this->easyhandle = curl_easy_init();
            
                if(!easyhandle)
                {        
                    std::cout << "Failed to init easy curl.";
                }
                curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data); 
            } 
            ~HttpMessenger()
            {
               curl_easy_cleanup(easyhandle); 
               curl_global_cleanup();   
            }

           //Http POST
           void post(std::vector<std::string> *, char *, std::string); 

           //Handles HTTP response
           static size_t write_data(void *returnedData, size_t size, size_t nmemb, void *userPointer){
              
                size_t new_length = size*nmemb;
                char * str = (char *) malloc(new_length+1);
                memcpy(str, returnedData, new_length);
                str[new_length] = '\0';

                std::string returnedXml (str);
                
                //Parse for <faultcode><faultstring><errorCode>
                std::vector<std::string>* keys = new std::vector<std::string>();
                keys->push_back("faultcode");
                keys->push_back("faultstring");
                keys->push_back("errorCode");
                std::vector<std::string> * value = getXmlByTags(keys, returnedXml);
                std::string err = std::accumulate(value->begin(), value->end(), std::string("")); 
            
                if( err.empty())
                {
                  std::cout << "VALID RETURN MESSAGE\n";
                }
                else
                {
                   throw std::runtime_error(err);
                }

                return size*nmemb;
           }
           
        };
        void HttpMessenger::post(std::vector<std::string> * headers, char *body, std::string url){

           CURLcode res;
          
           //Set POST url
           curl_easy_setopt(this->easyhandle, CURLOPT_URL, url.c_str());

           //Set HTTP headers
           struct curl_slist * curl_headers = NULL;
           for(std::vector<std::string>::iterator it = headers->begin(); it != headers->end();++it){
               curl_headers = curl_slist_append(curl_headers, (*it).c_str());
           }   
            
           curl_easy_setopt(this->easyhandle, CURLOPT_HTTPHEADER, curl_headers);

           //Set POST body
           curl_easy_setopt(this->easyhandle, CURLOPT_POSTFIELDS, body);
           curl_easy_setopt(this->easyhandle, CURLOPT_POSTFIELDSIZE, strlen(body));

           //Handle errors
           res = curl_easy_perform(this->easyhandle);
           if(res != CURLE_OK)
           {
              std::cout << curl_easy_strerror(res); 
           }
          
           //Free memory
           curl_slist_free_all(curl_headers); 
        }

