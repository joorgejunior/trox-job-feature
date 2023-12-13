#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <lwip/ip_addr.h>
// #include "DNSServer.h"
// #include <lwip/ip_addr.h>

const char OPEN_HEADER[] PROGMEM = "<html lang=\"pt-br\"> <head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=0\"> <title>Trox</title>";
const char STYLE[] PROGMEM = "<style>body, html{margin: 0;}div, form{display: flex; align-items: center;}.container{flex-direction: column; margin: 0 auto; background-color: white; max-width: 400px; padding-top: 100px;}h3{color: rgb(83, 83, 83); text-align: center; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; font-family: Arial, Helvetica, sans-serif;}form{justify-content: center; flex-direction: column; margin-top: 20px; width: 80%;}input{border: none; border-radius: 5px; background-color: #f5f5f5; height: 50px; width: 100%; margin-bottom: 10px; padding: 0 10px; outline: none;}button{border: none; border-radius: 5px; background-color: #1e1f7e; color: #f5f5f5; font-weight: bold; height: 50px; width: 50%;}</style>";
const char SCRIPT[] PROGMEM = "<script>function c(l){document.getElementById(\"ssid\").value=l.innerText||l.textContent; document.getElementById(\"pass\").focus();}</script> </head>";
const char BODY_START[] PROGMEM = "<body> <div class=\"container\">";
const char BODY_END[] PROGMEM = "</div> </body></html>";
const char MENU[] PROGMEM = "<form method=\"get\" action=\"wifi\"> <button type=\"submit\">Wi-Fi</button></form><form method=\"get\" action=\"network\"> <button type=\"submit\">Configuração de Rede</button> </form><form method=\"get\" action=\"config\"> <button type=\"submit\">Configuração MQTT</button> </form> <form method=\"get\" action=\"config-slave\"> <button type=\"submit\">Configuração RS485</button> </form>";
const char SEARCH_PAGE[] PROGMEM = "<form method=\"get\" action=\"sendWiFi\"> <input name=\"ssid\" type=\"text\" id=\"ssid\" placeholder=\"SSID\" required> <input name=\"pass\" type=\"password\" id=\"pass\" placeholder=\"Senha\"> <button type=\"submit\">Enviar</button> </form>";
const char COPY_PAGE[] PROGMEM = "<h3>Wi-Fi enviado!</h3> <h3>O dispositivo será reiniciado em breve!</h3><script>setTimeout(() => history.pushState({}, \"\", \"/\"), 2000)</script>";
const char CONFIG_PAGE[] PROGMEM = "<form method=\"get\" action=\"reset\"><h3>Configuração MQTT</h3><input name=\"server\" type=\"text\" id=\"server\" placeholder=\"Server MQTT\"><input name=\"port\" type=\"text\" id=\"port\" placeholder=\"Port MQTT (Default: 1883)\"><input name=\"topic\" type=\"text\" id=\"topic\" placeholder=\"Tópico publicação MQTT\"><input name=\"sub\" type=\"text\" id=\"sub\" placeholder=\"Tópico inscrição MQTT\"><input name=\"time\" type=\"number\" id=\"time\" placeholder=\"Tempo para envio (Segundos)\"><button type=\"submit\">Salvar</button> </form>";
const char CONFIG_NETWORK[] PROGMEM = "<form method=\"get\" action=\"setup\"> <h3>Configuração de Rede</h3> <input name=\"dhcp\" type=\"number\" max=\"1\" id=\"dhcp\" placeholder=\"enable DHCP (0 / 1)\"> <input name=\"ip\" type=\"text\" max=\"16\" id=\"ip\" placeholder=\"IP 0.0.0.0\"> <input name=\"mask\" type=\"text\" max=\"16\" id=\"mask\" placeholder=\"SUBNET MASK 0.0.0.0\"> <input name=\"dns\" type=\"text\" max=\"16\" id=\"dns\" placeholder=\"DNS 0.0.0.0\"> <input name=\"gateway\" type=\"text\" max=\"16\" id=\"gateway\" placeholder=\"GATEWAY 0.0.0.0\"><button type=\"submit\">Salvar</button></form>";
const char CONFIG_SLAVE[] PROGMEM = "<form method=\"get\" action=\"resets\"> <h3>Configuração RS485</h3> <input name=\"speed\" type=\"number\" id=\"speed\" placeholder=\"Speed\"> <input name=\"nodeid\" type=\"number\" id=\"nodeid\" placeholder=\"Slave ID (1 a 255)\">  <input name=\"registers\" type=\"number\" max=\"200\" id=\"registers\" placeholder=\"Quantidade de registradores\"><button type=\"submit\">Salvar</button></form>";
const char IMG_WIFI[] PROGMEM = "<img class=\"wifi-image\" src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAQAAADZc7J/AAAABGdBTUEAALGPC/xhBQAAACBjSFJNAAB6JgAAgIQAAPoAAACA6AAAdTAAAOpgAAA6mAAAF3CculE8AAAAAmJLR0QA/4ePzL8AAAAHdElNRQfmAhALNQCIdHuXAAACiUlEQVRIx+3US2xUdRQG8F/ttPRhFxQMdLALS4EyYUNkAQkbWdCoJBgTasKGsCMNhKASnrHQ6KKhYcUOiRKjCw1EI4nauBgTE9nwiItSSsvLodYSOyj2SqDDn83tZdoOpcjW7+zO/37nO8/L/3hulJX01mqSsUSDOtz1uyt6XTU2m5BNduoxbFwosnHDeuy0eGZy2kGXPYxJkRE5OSOiJFC/A9KlSyjTqsNqkHPWzy4Z9g9e1KDFWmssAr84rEeYrF1huxFBMKjDChXTsquwQocBQTBix+QvKu0zJrjnU5nYVydjvU02aZVRF3szPnFPENmv8nGAbSLBqF2qQYt9soZECgoiQ7L2Wgaq7TIqiGyboFf7QZC3VRnq7TE4aQITNmiPepTZKi/4XhXlGEeNo04Kmh3Trl5wzXe+8LmvnXVbrbnmWqfFBX/61R+qfObC4yKqQLOsILhmr8XKk9dyzQ64IfhXtzlFjCn796UgyFqVNDJtUdLANY5508vSFsZBpqDBZUE23raMTllXDPhJp+XxKN9wSp8+p2woyjBJtE2X5Si3JZ74hA14G+3uJJ472p9wSdjiL0FBr9NO61UQ7LDSkCAY9a0+wS2vlqYv0S/42wcapaQ0OuS6VfbH2p1e8Jq84CCpEgFq1Co44iMPwW8+dNOwpvg9JDf0SukMUl63Oe79fPPi/lT7Ks5g1Bn9gqDbU/CO885pQ8pud6dsZmTDzPT5zguCLnPsNjZttU+omTnAPOcExy3wfkK/77YH7svp9tLTCqBNlwXeK1If0mqjt2Smr1FpVHo3oY/5xqH44GeNdNFp5zSWGtnMSMW/rsiPLso/mzo0yj1ZfTYZ5H1sKfr/i/qs8Aj0eg/YToyhdAAAACV0RVh0ZGF0ZTpjcmVhdGUAMjAyMi0wMi0xNlQxMTo1Mjo1NyswMDowMDozf0IAAAAldEVYdGRhdGU6bW9kaWZ5ADIwMjItMDItMTZUMTE6NTI6NTcrMDA6MDBLbsf+AAAAAElFTkSuQmCC\" alt=\"\" />";
const char RESET_PAGE[] PROGMEM = "<h3>Configurações enviadas!</h3> <h3>Reiniciando...</h3><script>setTimeout(() => history.pushState({}, \"\", \"/\"), 2000)</script>";

class WebConfig{
  public:
    /**
     * @brief Start config mode ESP32
     * 
     * @param ssid for soft AP
     * @param timeout for soft AP (in MS)
     * @return bool
     */
    bool start(const char * ssid, unsigned int timeout);
    void stop();
  private:
    bool state {false};
};

/**
 * @brief Update networks page
 * 
 * @param page 
 */
void refreshNetworks(String * page);
/**
 * @brief Page search networks
 * 
 */
// void searchPage();
/**
 * @brief Page copy UUID
 * 
 */
// void copyPage();

// void configPage();
// void resetPage();
// void menuPage();