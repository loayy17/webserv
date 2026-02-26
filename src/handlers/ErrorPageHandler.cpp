#include "ErrorPageHandler.hpp"
#include <fstream>
#include <sstream>

ErrorPageHandler::ErrorPageHandler() {}

ErrorPageHandler::ErrorPageHandler(const ErrorPageHandler& other) {
    (void)other;
}

ErrorPageHandler& ErrorPageHandler::operator=(const ErrorPageHandler& other) {
    (void)other;
    return *this;
}

ErrorPageHandler::~ErrorPageHandler() {}


std::string ErrorPageHandler::generateHtml(int code, const std::string& msg) const {
    std::string codeStr = typeToString<int>(code);

    return "<!DOCTYPE html>\n"
           "<html>\n"
           "<head>\n"
           "<meta charset=\"utf-8\">\n"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "<title>" +
           codeStr + " " + msg +
           "</title>\n"
           "<style>\n"

           "html,body{height:100%;margin:0;}\n"
           "body{display:flex;justify-content:center;align-items:center;"
           "background:linear-gradient(-45deg,#0f172a,#1e293b,#0f172a,#111827);"
           "background-size:400% 400%;animation:bgMove 12s ease infinite;"
           "font-family:monospace;color:#fff;overflow:hidden;}\n"

           "@keyframes bgMove{0%{background-position:0% 50%;}"
           "50%{background-position:100% 50%;}"
           "100%{background-position:0% 50%;}}\n"

           "#container{text-align:center;z-index:2;}\n"

           ".number{font-size:6rem;font-weight:700;letter-spacing:4px;"
           "animation:pulse 1.5s ease-in-out infinite;}\n"

           "@keyframes pulse{0%{transform:scale(1);"
           "text-shadow:0 0 10px rgba(255,255,255,.3);}"
           "50%{transform:scale(1.08);"
           "text-shadow:0 0 25px rgba(96,165,250,.9);}"
           "100%{transform:scale(1);"
           "text-shadow:0 0 10px rgba(255,255,255,.3);}}\n"

           ".text{margin:.6rem 0;font-size:1.1rem;opacity:.85;}\n"

           ".home{display:inline-block;margin-top:1.5rem;padding:.8rem 2rem;"
           "border-radius:30px;background:#3b82f6;color:#fff;"
           "text-decoration:none;font-weight:600;transition:.3s;}\n"

           ".home:hover{background:#2563eb;transform:translateY(-3px);"
           "box-shadow:0 10px 25px rgba(37,99,235,.5);}\n"

           ".particle{position:absolute;width:4px;height:4px;"
           "background:rgba(255,255,255,.5);border-radius:50%;"
           "animation:float linear infinite;}\n"

           "@keyframes float{from{transform:translateY(100vh);opacity:0;}"
           "10%{opacity:1;}to{transform:translateY(-10vh);opacity:0;}}\n"

           "</style>\n"
           "</head>\n"
           "<body>\n"

           "<div id=\"container\">\n"
           "<div class=\"number\" data-count=\"" +
           codeStr +
           "\">0</div>\n"
           "<div class=\"text\">" +
           msg +
           "</div>\n"
           "<div class=\"text\">The server responded with status " +
           codeStr +
           ".</div>\n"
           "<a class=\"home\" href=\"/\">Go Home</a>\n"
           "</div>\n"

           "<script>\n"
           "const number=document.querySelector('.number');"
           "const target=parseInt(number.dataset.count);"
           "let current=0;"
           "const duration=1200;"
           "const step=Math.max(1,Math.floor(duration/target));"
           "const counter=setInterval(()=>{"
           "current++;number.textContent=current;"
           "if(current>=target)clearInterval(counter);"
           "},step);"

           "for(let i=0;i<40;i++){"
           "const p=document.createElement('div');"
           "p.className='particle';"
           "p.style.left=Math.random()*100+'vw';"
           "p.style.animationDuration=(5+Math.random()*10)+'s';"
           "p.style.animationDelay=Math.random()*5+'s';"
           "document.body.appendChild(p);"
           "}"
           "</script>\n"

           "</body>\n"
           "</html>\n";
}

void ErrorPageHandler::handle(HttpResponse& response, const RouteResult& resultRouter, const MimeTypes& mimeTypes) const {
    int         code = resultRouter.getStatusCode() ? resultRouter.getStatusCode() : 500;
    std::string msg  = resultRouter.getErrorMessage().empty() ? getHttpStatusMessage(code) : resultRouter.getErrorMessage();

    std::string body;
    response.addHeader(HEADER_SERVER, "Webserv/1.0");
    // Check for custom error page in location first, then server
    const std::string customPage = resultRouter.getLocation() ? resultRouter.getLocation()->getErrorPage(code) : "";
    if (!customPage.empty() && readFileContent(customPage, body)) {
        response.setStatus(code, msg);
        response.addHeader("Content-Type", mimeTypes.get(customPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        if (resultRouter.getRequest().getMethod() != "HEAD")
            response.setBody(body);
        
        return;
    }

    const std::string serverPage = resultRouter.getServer() ? resultRouter.getServer()->getErrorPage(code) : "";
    if (!serverPage.empty() && readFileContent(serverPage, body)) {
        response.setStatus(code, msg);
        response.addHeader("Content-Type", mimeTypes.get(serverPage));
        response.addHeader("Content-Length", typeToString<size_t>(body.size()));
        if (resultRouter.getRequest().getMethod() != "HEAD") 
            response.setBody(body);
        
        return;
    }

    // Fallback HTML
    body = generateHtml(code, msg);
    response.setStatus(code, msg);
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Content-Length", typeToString<size_t>(body.size()));
    if (resultRouter.getRequest().getMethod() != "HEAD") 
        response.setBody(body);
    
}
