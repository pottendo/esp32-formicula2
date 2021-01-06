/* -*-c++-*-
 * This file is part of formicula2.
 * 
 * vice-mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * vice-mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with vice-mapper.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include <Arduino.h>
#include <AutoConnect.h>
#include <PageBuilder.h>

#include "ui.h"

//    static PageElement ROOT_PAGE_ELEMENT(rp->c_str());
//    static PageBuilder ROOT_PAGE("/", {ROOT_PAGE_ELEMENT});

static PageElement elm;
static PageBuilder page;
static bool handle_web(HTTPMethod method, String uri);
static String currentUri;
static uiElements *ui;

void setup_web(WebServer &ip_server, uiElements *u)
{
    ui = u;
    page.exitCanHandle(handle_web); // Handles for all requests.
    page.insert(ip_server);
}

// The root page content builder
String rootPage(PageArgument &args)
{
    return String(F("Formicula Control Centre..."));
}

String body(PageArgument &args)
{
    return String{String("<a href=\"http://" + WiFi.localIP().toString() + "/_ac\">FCC Administration</a>")};
}

String body_fcc_ut(PageArgument &args)
{
    return ui->get_fcc_ut();
}

String body_fcce_ut(PageArgument &args)
{
    return ui->get_fcce_ut();
}

String bodyLog_msg(PageArgument &args)
{
    return get_log(myLogger::LOG_MSG);
}

String bodyLog_sensor(PageArgument &args)
{
    return get_log(myLogger::LOG_SENSOR);
}

String bodyLog_circuit(PageArgument &args)
{
    return get_log(myLogger::LOG_CIRCUIT);
}

String _token_CSS_BASE(PageArgument &args)
{
    return String(FPSTR(AutoConnect::_CSS_BASE));
}
String _token_CSS_TABLE(PageArgument &args)
{
    return String(FPSTR(AutoConnect::_CSS_TABLE));
}

static bool handle_web(HTTPMethod method, String uri)
{
    if (uri == currentUri)
    {
        // Page is already prepared.
        return true;
    }
    else
    {
        currentUri = uri;
        page.clearElement();  // Discards the remains of PageElement.
        page.addElement(elm); // Register PageElement for current access.

        Serial.println("Request:" + uri);

        if (uri == "/")
        { // for the / page
            page.setUri(uri.c_str());
            elm.setMold(PSTR(
                "<!DOCTYPE html>"
                "<html>"
                "<head>"
                "<meta charset=\"UTF-8\" name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                "<style type=\"text/css\">"
                "{{CSS_BASE}}"
                "{{CSS_TABLE}}"
                "</style>"
                "<body style=\"padding-top:58px;\">"
                 "<h2>{{ROOT}}</h2>"
                  "<div class=\"container\">"
                  "<p>{{BODY_HEAD}}</p>"
                  "<p>FCC Uptime: {{BODY_FCCUT}}</p>"
                  "<p>FCCE Uptime: {{BODY_FCCEUT}}</p>"
                  "<h3>FCC Message Log:</h3>"
                    "<div>"
                    "<table class=\"info\">"
                        "<tbody>"
                        "{{LOG_MSG}}"
                        "</tbody>"
                    "</table>"
                    "</div>"   
                  "<h3>FCC Sensor Log:</h3>"
                    "<div>"
                    "<table class=\"info\">"
                        "<tbody>"
                        "{{LOG_SENSOR}}"
                        "</tbody>"
                    "</table>"
                    "</div>"   
                  "<h3>FCC Circuit Log:</h3>"
                    "<div>"
                    "<table class=\"info\">"
                        "<tbody>"
                        "{{LOG_CIRCUIT}}"
                        "</tbody>"
                    "</table>"
                    "</div>"   
                  "</div>"   
                "</body>"
                "</html>"));
            elm.addToken("CSS_BASE", _token_CSS_BASE);
            elm.addToken("CSS_TABLE", _token_CSS_TABLE);
            elm.addToken("ROOT", rootPage);
            elm.addToken("BODY_HEAD", body);
            elm.addToken("BODY_FCCUT", body_fcc_ut);
            elm.addToken("BODY_FCCEUT", body_fcce_ut);
            elm.addToken("LOG_MSG", bodyLog_msg);
            elm.addToken("LOG_SENSOR", bodyLog_sensor);
            elm.addToken("LOG_CIRCUIT", bodyLog_circuit);
            return true;
        }
        /*
        else if (uri == "/hello")
        { // for the /hello page
            page.setUri(uri.c_str());
            elm.setMold(PSTR(
                "<html>"
                "<body>"
                "<p style=\"color:Tomato;\">{{HELLO}}</p>"
                "</body>"
                "</html>"));
            elm.addToken("HELLO", helloPage);
            return true;
        }
        */
        else
        {
            return false; // Not found to accessing exception URI.
        }
    }
}