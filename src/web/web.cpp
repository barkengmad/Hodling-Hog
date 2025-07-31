#include "web.h"
#include "../settings/settings.h"
#include "../wallet/wallet.h"
#include "../cold/cold.h"

// External function declarations from main.cpp
extern void updateBalances();
extern bool wifiConnected;
extern unsigned long lastInputTime;

// External instances
extern class LightningWallet lightningWallet;

// Activity tracking helper
void updateWebActivity() {
    lastInputTime = millis();
}

// Global instance
WebInterface webInterface;

WebInterface::WebInterface() : server(WEB_SERVER_PORT) {
    status = WebStatus::STOPPED;
    captivePortalEnabled = false;
    apModeActive = false;
    apSSID = "HodlingHog-Config";
    apPassword = "hodling123";
    apIP = IPAddress(192, 168, 4, 1);
    apGateway = IPAddress(192, 168, 4, 1);
    apSubnet = IPAddress(255, 255, 255, 0);
    lastSessionCleanup = 0;
    apTimeout = CONFIG_AP_TIMEOUT;
    maxClients = MAX_CLIENTS;
    authRequired = false;
    adminPassword = "admin123";
    pendingSeedPhrase = "";
}

void WebInterface::init() {
    Serial.println("WebInterface: Initializing");
    setupRoutes();
    status = WebStatus::STOPPED;
}

void WebInterface::start() {
    Serial.println("WebInterface: Starting web server");
    server.begin();
    status = WebStatus::RUNNING;
}

void WebInterface::stop() {
    Serial.println("WebInterface: Stopping web server");
    server.end();
    status = WebStatus::STOPPED;
}

void WebInterface::loop() {
    // Handle session cleanup
    if (millis() - lastSessionCleanup > 60000) { // Every minute
        cleanupSessions();
        lastSessionCleanup = millis();
    }
}

void WebInterface::enableCaptivePortal(bool enable) {
    captivePortalEnabled = enable;
    Serial.printf("WebInterface: Captive portal %s\n", enable ? "enabled" : "disabled");
}

void WebInterface::startAPMode() {
    Serial.println("WebInterface: Starting AP mode");
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    apModeActive = true;
    status = WebStatus::AP_MODE;
    
    Serial.printf("WebInterface: AP started - SSID: %s, IP: %s\n", 
                  apSSID.c_str(), apIP.toString().c_str());
}

void WebInterface::stopAPMode() {
    Serial.println("WebInterface: Stopping AP mode");
    WiFi.softAPdisconnect(true);
    apModeActive = false;
    status = WebStatus::STOPPED;
}

String WebInterface::getAPIP() const {
    return apIP.toString();
}

// Stub implementations for handlers
void WebInterface::handleConfigRequest(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: Config request");
    request->send(200, "text/html; charset=utf-8", generateConfigPage());
}

void WebInterface::handleApiRequest(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: API request");
    request->send(200, "application/json; charset=utf-8", "{\"status\":\"ok\"}");
}

void WebInterface::handleFileUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
    Serial.printf("WebInterface: File upload - %s\n", filename.c_str());
}

void WebInterface::handleWalletConfig(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: Wallet config");
    request->send(200, "text/html; charset=utf-8", "Wallet Config");
}

void WebInterface::handleLightningTransfer(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: Lightning transfer");
    request->send(200, "text/html; charset=utf-8", "Lightning Transfer");
}

void WebInterface::handleWiFiConfig(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    Serial.println("WebInterface: Processing WiFi config form");
    
    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    String ssid = "";
    String password = "";
    
    if (request->hasParam("ssid", true)) {
        ssid = request->getParam("ssid", true)->value();
    }
    if (request->hasParam("password", true)) {
        password = request->getParam("password", true)->value();
    }
    
    if (ssid.length() > 0) {
        if (settings.setWiFiCredentials(ssid, password)) {
            if (settings.saveConfig()) {
                Serial.printf("WiFi config saved - SSID: %s\n", ssid.c_str());
                request->redirect("/config?saved=wifi");
                return;
            }
        }
    }
    
    request->redirect("/config?error=wifi");
}

void WebInterface::handleLightningConfig(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    Serial.println("WebInterface: Processing Lightning config form");
    
    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    String apiToken = "";
    String apiSecret = "";
    String lightningAddress = "";
    
    if (request->hasParam("api_token", true)) {
        apiToken = request->getParam("api_token", true)->value();
    }
    if (request->hasParam("api_secret", true)) {
        apiSecret = request->getParam("api_secret", true)->value();
    }
    if (request->hasParam("lightning_address", true)) {
        lightningAddress = request->getParam("lightning_address", true)->value();
    }
    
    // Validate required fields
    if (apiToken.length() > 0 && apiSecret.length() > 0) {
        // Save all Lightning credentials
        if (settings.setLightningCredentials(apiToken, apiSecret, lightningAddress)) {
            if (settings.saveConfig()) {
                // Update the Lightning wallet instance with new credentials
                lightningWallet.setApiToken(apiToken);
                lightningWallet.setApiSecret(apiSecret);
                
                Serial.printf("Lightning config saved - Token: %s***, Secret: %s***, Address: %s\n", 
                             apiToken.substring(0, 8).c_str(), 
                             apiSecret.substring(0, 8).c_str(),
                             lightningAddress.c_str());
                request->redirect("/config?saved=lightning");
                return;
            }
        }
    } else {
        Serial.println("WebInterface: Missing required Lightning credentials (token or secret)");
    }
    
    request->redirect("/config?error=lightning");
}

void WebInterface::handleColdStorageConfig(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    Serial.println("=== HANDLER DEBUG: Cold Storage handler START ===");
    Serial.printf("Request URL: %s\n", request->url().c_str());
    Serial.printf("Method: %s\n", request->method() == HTTP_POST ? "POST" : "GET");
    Serial.printf("Request object address: %p\n", request);
    
    // Add safety check for null request
    if (!request) {
        Serial.println("ERROR: Null request received");
        return;
    }
    Serial.println("HANDLER DEBUG: Request object is valid");
    
    try {
        Serial.println("HANDLER DEBUG: Entered try block");
        
        if (request->method() != HTTP_POST) {
            Serial.println("HANDLER DEBUG: Wrong method - sending 405");
            request->send(405, "text/plain", "Method Not Allowed");
            Serial.println("HANDLER DEBUG: 405 response sent");
            return;
        }
        Serial.println("HANDLER DEBUG: Method check passed");
        
        String address = "";
        Serial.println("HANDLER DEBUG: About to check for address parameter");
        
        // Check if parameter exists
        if (request->hasParam("address", true)) {
            address = request->getParam("address", true)->value();
            Serial.printf("HANDLER DEBUG: Received address: %s\n", address.c_str());
        } else {
            Serial.println("HANDLER DEBUG: No address parameter found");
        }
        
        Serial.printf("HANDLER DEBUG: Address length: %d\n", address.length());
        
        if (address.length() > 0) {
            Serial.println("HANDLER DEBUG: Attempting to save address...");
            
            // Check if settings is initialized
            Serial.println("HANDLER DEBUG: About to check settings availability");
            if (!settings.getConfig().wifi.ssid.isEmpty() || true) { // Basic check that settings exists
                Serial.println("HANDLER DEBUG: Settings appears to be available");
                
                Serial.println("HANDLER DEBUG: Calling setColdStorageAddress...");
                if (settings.setColdStorageAddress(address)) {
                    Serial.println("HANDLER DEBUG: setColdStorageAddress succeeded");
                    
                    // IMPORTANT: Update the cold storage instance with the new address!
                    Serial.println("HANDLER DEBUG: Updating coldStorage instance with new address...");
                    coldStorage.setAddress(address);
                    Serial.println("HANDLER DEBUG: Cold storage instance updated");
                    
                    // CRITICAL: Update balance immediately with the new address!
                    Serial.println("HANDLER DEBUG: Triggering balance update...");
                    coldStorage.updateBalance();
                    Serial.println("HANDLER DEBUG: Balance update completed");
                    
                    Serial.println("HANDLER DEBUG: Calling saveConfig...");
                    if (settings.saveConfig()) {
                        Serial.printf("HANDLER DEBUG: Cold storage address saved successfully: %s\n", address.c_str());
                        Serial.println("HANDLER DEBUG: About to redirect to success page...");
                        request->redirect("/config?saved=coldstorage");
                        Serial.println("HANDLER DEBUG: Redirect sent successfully");
                        return;
                    } else {
                        Serial.println("HANDLER DEBUG: saveConfig failed");
                    }
                } else {
                    Serial.println("HANDLER DEBUG: setColdStorageAddress failed");
                }
            } else {
                Serial.println("HANDLER DEBUG: Settings not available");
            }
        } else {
            Serial.println("HANDLER DEBUG: Empty address provided");
        }
        
        Serial.println("HANDLER DEBUG: About to redirect to error page");
        request->redirect("/config?error=coldstorage");
        Serial.println("HANDLER DEBUG: Error redirect sent");
        
    } catch (const std::exception& e) {
        Serial.printf("HANDLER DEBUG: Exception caught: %s\n", e.what());
        request->send(500, "text/plain", "Internal Server Error");
        Serial.println("HANDLER DEBUG: Exception response sent");
    } catch (...) {
        Serial.println("HANDLER DEBUG: Unknown exception caught");
        request->send(500, "text/plain", "Internal Server Error");
        Serial.println("HANDLER DEBUG: Unknown exception response sent");
    }
    
    Serial.println("=== HANDLER DEBUG: Cold Storage handler END ===");
}

void WebInterface::handleSystemConfig(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    Serial.println("WebInterface: Processing system config form");
    
    if (request->method() != HTTP_POST) {
        request->send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    bool hasChanges = false;
    
    // Get owner name parameter
    if (request->hasParam("ownerName", true)) {
        String ownerName = request->getParam("ownerName", true)->value();
        ownerName.trim(); // Remove leading/trailing spaces
        
        // Validate name (allow empty for default)
        if (ownerName.length() <= 20) {
            if (ownerName.isEmpty()) {
                ownerName = "Hodling Hog"; // Reset to default
            }
            
            // Set device name directly in config
            settings.getConfig().system.deviceName = ownerName;
            hasChanges = true;
            Serial.printf("WebInterface: Owner name updated to: %s\n", ownerName.c_str());
        } else {
            Serial.printf("WebInterface: Invalid owner name length: %d\n", ownerName.length());
            request->redirect("/config?error=system");
            return;
        }
    }
    
    // Get sleep timeout parameter
    if (request->hasParam("sleepTimeout", true)) {
        String sleepTimeoutStr = request->getParam("sleepTimeout", true)->value();
        uint32_t sleepTimeoutMinutes = sleepTimeoutStr.toInt();
        
        // Validate range (1-60 minutes)
        if (sleepTimeoutMinutes >= 1 && sleepTimeoutMinutes <= 60) {
            uint32_t sleepTimeoutMs = sleepTimeoutMinutes * 60000; // Convert to milliseconds
            
            if (settings.setSleepTimeout(sleepTimeoutMs)) {
                hasChanges = true;
                Serial.printf("WebInterface: Sleep timeout updated to %u minutes (%u ms)\n", 
                             sleepTimeoutMinutes, sleepTimeoutMs);
            } else {
                Serial.println("WebInterface: Failed to save sleep timeout setting");
                request->redirect("/config?error=system");
                return;
            }
        } else {
            Serial.printf("WebInterface: Invalid sleep timeout: %u minutes\n", sleepTimeoutMinutes);
            request->redirect("/config?error=system");
            return;
        }
    }
    
    if (hasChanges) {
        if (settings.saveConfig()) {
            Serial.println("WebInterface: System settings saved successfully");
            request->redirect("/config?saved=system");
        } else {
            Serial.println("WebInterface: Failed to save system settings");
            request->redirect("/config?error=system");
        }
    } else {
        Serial.println("WebInterface: No system settings parameters found");
        request->redirect("/config?error=system");
    }
}

void WebInterface::handleTransactionSigning(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: Transaction signing");
    request->send(200, "text/html; charset=utf-8", "Transaction Signing");
}

void WebInterface::handleSystemInfo(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: System info");
    request->send(200, "application/json; charset=utf-8", getSystemStatus());
}

void WebInterface::handleSystemRestart(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: System restart");
    request->send(200, "text/html; charset=utf-8", "Restarting...");
    delay(1000);
    ESP.restart();
}

void WebInterface::handleFactoryReset(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: ⚠️ FACTORY RESET INITIATED ⚠️");
    updateWebActivity(); // Reset sleep timer
    
    // Clear all active sessions immediately
    clearAllSessions();
    Serial.println("WebInterface: All sessions cleared");
    
    // Reset all settings to factory defaults
    if (settings.factoryReset()) {
        Serial.println("WebInterface: Settings reset to factory defaults");
    } else {
        Serial.println("WebInterface: WARNING - Settings reset may have failed");
    }
    
    // Send confirmation page with redirect to seed generation
    String html = "<!DOCTYPE html>";
    html += "<html><head><title>Factory Reset Complete</title>";
    html += "<meta http-equiv='refresh' content='3;url=/generate-seed'>";
    html += "<style>";
    html += "body{font-family:Arial;text-align:center;padding:2rem;background:#ffecd2;}";
    html += ".reset-container{max-width:600px;margin:0 auto;background:white;border-radius:20px;padding:2rem;box-shadow:0 4px 6px rgba(0,0,0,0.1);}";
    html += ".reset-title{font-size:2rem;color:#f44336;margin-bottom:1rem;}";
    html += ".reset-message{font-size:1.2rem;color:#333;margin-bottom:2rem;}";
    html += ".countdown{font-size:1rem;color:#666;}";
    html += "</style></head><body>";
    html += "<div class='reset-container'>";
    html += "<div class='reset-title'>🗑️ Factory Reset Complete</div>";
    html += "<div class='reset-message'>";
    html += "All data has been permanently erased:<br>";
    html += "• Seed phrase and login<br>";
    html += "• Lightning wallet data<br>";
    html += "• Cold storage settings<br>";
    html += "• WiFi configuration<br>";
    html += "• System settings";
    html += "</div>";
    html += "<div class='countdown'>Redirecting to setup in 3 seconds...</div>";
    html += "</div></body></html>";
    
    request->send(200, "text/html; charset=utf-8", html);
    Serial.println("WebInterface: Factory reset complete - redirecting to seed generation");
}

void WebInterface::handleFirmwareUpdate(AsyncWebServerRequest* request) {
    Serial.println("WebInterface: Firmware update");
    request->send(200, "text/html; charset=utf-8", "Firmware Update");
}

bool WebInterface::authenticateRequest(AsyncWebServerRequest* request, AuthLevel requiredLevel) {
    Serial.printf("AUTH DEBUG: Checking auth for %s, required level: %d\n", 
                  request->url().c_str(), (int)requiredLevel);
    
    // Check if authentication is required
    if (!settings.isSeedPhraseSet()) {
        Serial.println("AUTH DEBUG: No seed phrase set, allowing access");
        return true; // No auth configured yet
    }
    
    // Check for session token in cookie or header
    String sessionToken = "";
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        Serial.printf("AUTH DEBUG: Cookie header: %s\n", cookie.c_str());
        int sessionStart = cookie.indexOf("session=");
        if (sessionStart != -1) {
            sessionStart += 8; // Length of "session="
            int sessionEnd = cookie.indexOf(";", sessionStart);
            if (sessionEnd == -1) sessionEnd = cookie.length();
            sessionToken = cookie.substring(sessionStart, sessionEnd);
            Serial.printf("AUTH DEBUG: Found session token: %s\n", sessionToken.c_str());
        } else {
            Serial.println("AUTH DEBUG: No session cookie found");
        }
    } else {
        Serial.println("AUTH DEBUG: No Cookie header");
    }
    
    // Check authorization header for seed phrase
    if (sessionToken.isEmpty() && request->hasHeader("Authorization")) {
        String auth = request->header("Authorization");
        if (auth.startsWith("Bearer ")) {
            String seedPhrase = auth.substring(7); // Remove "Bearer "
            if (settings.validateSeedPhrase(seedPhrase)) {
                // Create session for this request
                WebContext* context = createSession(getClientIP(request), 
                                                   request->hasHeader("User-Agent") ? 
                                                   request->header("User-Agent") : "Unknown");
                if (context) {
                    context->authLevel = AuthLevel::ADMIN;
                    sessionToken = context->sessionToken;
                }
            }
        }
    }
    
    // Validate session token
    if (!sessionToken.isEmpty() && validateSessionToken(sessionToken)) {
        WebContext* session = getSession(sessionToken);
        if (session && session->authLevel >= requiredLevel) {
            Serial.printf("AUTH DEBUG: Authentication successful, user level: %d\n", (int)session->authLevel);
            updateSessionActivity(sessionToken);
            return true;
        } else if (session) {
            Serial.printf("AUTH DEBUG: Insufficient privileges - user level: %d, required: %d\n", 
                         (int)session->authLevel, (int)requiredLevel);
        } else {
            Serial.println("AUTH DEBUG: Session not found");
        }
    } else {
        Serial.println("AUTH DEBUG: No valid session token");
    }
    
    // Authentication failed
    Serial.println("AUTH DEBUG: Authentication failed");
    return false;
}

String WebInterface::generateSessionToken() {
    return "session_" + String(millis());
}

bool WebInterface::validateSessionToken(const String& token) {
    return activeSessions.find(token) != activeSessions.end();
}

void WebInterface::invalidateSession(const String& token) {
    activeSessions.erase(token);
}

void WebInterface::clearAllSessions() {
    Serial.printf("WebInterface: Clearing all sessions (%d active)\n", activeSessions.size());
    activeSessions.clear();
    Serial.println("WebInterface: All sessions cleared");
}

String WebInterface::getDeviceInfo() {
    return "Hodling Hog v1.0";
}

String WebInterface::getSystemStatus() {
    return "{\"status\":\"running\",\"uptime\":" + String(millis()) + ",\"free_heap\":" + String(ESP.getFreeHeap()) + "}";
}

String WebInterface::getNetworkInfo() {
    return "{\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + "}";
}

// Private methods - stubs
void WebInterface::setupRoutes() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleRoot(request);
    });
    
    server.on("/login", HTTP_GET, [this](AsyncWebServerRequest* request) {
        request->send(200, "text/html; charset=utf-8", generateLoginPage());
    });
    
    server.on("/login", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleLogin(request);
    });
    
    server.on("/logout", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleLogout(request);
    });
    
    server.on("/setup", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (settings.isSeedPhraseSet()) {
            request->redirect("/");
            return;
        }
        request->send(200, "text/html; charset=utf-8", generateSetupPage());
    });
    
    server.on("/setup", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleSetup(request);
    });
    
    server.on("/generate-seed", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (settings.isSeedPhraseSet()) {
            request->redirect("/");
            return;
        }
        handleGenerateSeed(request);
    });
    
    server.on("/confirm-seed", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (settings.isSeedPhraseSet() || pendingSeedPhrase.isEmpty()) {
            request->redirect("/");
            return;
        }
        request->send(200, "text/html; charset=utf-8", generateSeedConfirmPage());
    });
    
    server.on("/confirm-seed", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleConfirmSeed(request);
    });
    
    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::ADMIN)) {
            request->redirect("/login");
            return;
        }
        handleConfig(request);
    });
    

    

    
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleSystemInfo(request);
    });
    
    // IMPORTANT: Specific routes MUST come before general routes to avoid conflicts!
    // Move specific config endpoints BEFORE the general /api/config routes
    server.on("/api/config/wifi", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::BASIC)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        handleWiFiConfig(request);
    });
    
    server.on("/api/config/lightning", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::BASIC)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        handleLightningConfig(request);
    });
    
    server.on("/api/config/coldstorage", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::BASIC)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        handleColdStorageConfig(request);
    });
    
    server.on("/api/config/system", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::BASIC)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        handleSystemConfig(request);
    });
    
    server.on("/api/factory-reset", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::ADMIN)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        handleFactoryReset(request);
    });

    // General /api/config routes come AFTER specific ones
    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::ADMIN)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        apiGetConfig(request);
    });
    
    server.on("/api/config", HTTP_POST, [this](AsyncWebServerRequest* request) {
        if (!authenticateRequest(request, AuthLevel::ADMIN)) {
            sendErrorResponse(request, "Authentication required", 401);
            return;
        }
        apiSetConfig(request);
    });
    

    
    server.onNotFound([this](AsyncWebServerRequest* request) {
        handleNotFound(request);
    });
}

void WebInterface::handleRoot(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    
    // Check if seed phrase is configured
    if (!settings.isSeedPhraseSet()) {
        // Show landing page with setup option
        request->send(200, "text/html; charset=utf-8", generateLandingPage());
        return;
    }
    
    // Check authentication
    if (!authenticateRequest(request, AuthLevel::BASIC)) {
        // Show landing page with login option
        request->send(200, "text/html; charset=utf-8", generateLandingPage());
        return;
    }
    
    request->send(200, "text/html; charset=utf-8", generateMainPage());
}

void WebInterface::handleConfig(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    request->send(200, "text/html; charset=utf-8", generateConfigPage());
}

void WebInterface::handleAPI(AsyncWebServerRequest* request) {
    request->send(200, "application/json; charset=utf-8", "{\"message\":\"API endpoint\"}");
}

void WebInterface::handleNotFound(AsyncWebServerRequest* request) {
    request->send(404, "text/html; charset=utf-8", "Page not found");
}

void WebInterface::handleCaptivePortal(AsyncWebServerRequest* request) {
    request->redirect("http://" + apIP.toString());
}

void WebInterface::handleLogin(AsyncWebServerRequest* request) {
    updateWebActivity(); // Reset sleep timer on web activity
    
    if (request->method() == HTTP_POST) {
        // Get seed phrase from POST body
        String seedPhrase = "";
        if (request->hasParam("seedphrase", true)) {
            seedPhrase = request->getParam("seedphrase", true)->value();
        }
        
        if (settings.validateSeedPhrase(seedPhrase)) {
            // Create session
            WebContext* context = createSession(getClientIP(request), 
                                               request->hasHeader("User-Agent") ? 
                                               request->header("User-Agent") : "Unknown");
            if (context) {
                context->authLevel = AuthLevel::ADMIN;
                
                // Set session cookie
                AsyncWebServerResponse* response = request->beginResponse(302);
                response->addHeader("Location", "/");
                response->addHeader("Set-Cookie", "session=" + context->sessionToken + "; Path=/; HttpOnly; Max-Age=1800");
                request->send(response);
                
                Serial.println("WebInterface: User logged in successfully");
                
                // Update balances on successful login to show fresh data
                if (wifiConnected) {
                    Serial.println("Triggering balance update on login");
                    updateBalances();
                }
                return;
            }
        }
        
        // Login failed
        Serial.println("WebInterface: Login failed - invalid seed phrase");
        String loginPage = generateLoginPage();
        loginPage.replace("{{error}}", "<div class='error'>Invalid seed phrase or account locked</div>");
        request->send(401, "text/html; charset=utf-8", loginPage);
    } else {
        // Show login form
        request->send(200, "text/html; charset=utf-8", generateLoginPage());
    }
}

void WebInterface::handleLogout(AsyncWebServerRequest* request) {
    // Get session token and invalidate it
    String sessionToken = "";
    if (request->hasHeader("Cookie")) {
        String cookie = request->header("Cookie");
        int sessionStart = cookie.indexOf("session=");
        if (sessionStart != -1) {
            sessionStart += 8;
            int sessionEnd = cookie.indexOf(";", sessionStart);
            if (sessionEnd == -1) sessionEnd = cookie.length();
            sessionToken = cookie.substring(sessionStart, sessionEnd);
        }
    }
    
    if (!sessionToken.isEmpty()) {
        invalidateSession(sessionToken);
    }
    
    // Clear cookie and redirect to landing page
    AsyncWebServerResponse* response = request->beginResponse(302);
    response->addHeader("Location", "/");
    response->addHeader("Set-Cookie", "session=; Path=/; HttpOnly; Max-Age=0");
    request->send(response);
    
    Serial.println("WebInterface: User logged out");
}

void WebInterface::handleSetup(AsyncWebServerRequest* request) {
    if (settings.isSeedPhraseSet()) {
        request->redirect("/");
        return;
    }
    
    if (request->method() == HTTP_POST) {
        String seedPhrase = "";
        if (request->hasParam("seedphrase", true)) {
            seedPhrase = request->getParam("seedphrase", true)->value();
        }
        
        if (settings.setSeedPhrase(seedPhrase)) {
            // Seed phrase set successfully, auto-login
            WebContext* context = createSession(getClientIP(request), 
                                               request->hasHeader("User-Agent") ? 
                                               request->header("User-Agent") : "Unknown");
            if (context) {
                context->authLevel = AuthLevel::ADMIN;
                
                AsyncWebServerResponse* response = request->beginResponse(302);
                response->addHeader("Location", "/");
                response->addHeader("Set-Cookie", "session=" + context->sessionToken + "; Path=/; HttpOnly; Max-Age=1800");
                request->send(response);
                
                Serial.println("WebInterface: Seed phrase configured and user logged in");
                return;
            }
        }
        
        // Setup failed
        String setupPage = generateSetupPage();
        setupPage.replace("{{error}}", "<div class='error'>Invalid seed phrase format. Please check that you have exactly 12 valid words.</div>");
        request->send(400, "text/html; charset=utf-8", setupPage);
    } else {
        request->send(200, "text/html; charset=utf-8", generateSetupPage());
    }
}

void WebInterface::handleGenerateSeed(AsyncWebServerRequest* request) {
    if (settings.isSeedPhraseSet()) {
        request->redirect("/");
        return;
    }
    
    // Generate new kid-friendly seed phrase
    pendingSeedPhrase = settings.generateKidFriendlySeedPhrase();
    
    // Show the generated seed phrase to the user
    request->send(200, "text/html; charset=utf-8", generateSeedDisplayPage(pendingSeedPhrase));
    
    Serial.println("WebInterface: Generated seed phrase for new user");
}

void WebInterface::handleConfirmSeed(AsyncWebServerRequest* request) {
    if (settings.isSeedPhraseSet() || pendingSeedPhrase.isEmpty()) {
        request->redirect("/");
        return;
    }
    
    if (request->method() == HTTP_POST) {
        String enteredSeed = "";
        if (request->hasParam("seedphrase", true)) {
            enteredSeed = request->getParam("seedphrase", true)->value();
        }
        
        // Normalize both for comparison
        String normalizedEntered = settings.normalizeSeedPhrase(enteredSeed);
        String normalizedPending = settings.normalizeSeedPhrase(pendingSeedPhrase);
        
        if (normalizedEntered == normalizedPending) {
            // Confirmation successful - set the seed phrase
            if (settings.setSeedPhrase(pendingSeedPhrase)) {
                // Clear pending seed and auto-login
                pendingSeedPhrase = "";
                
                WebContext* context = createSession(getClientIP(request), 
                                                   request->hasHeader("User-Agent") ? 
                                                   request->header("User-Agent") : "Unknown");
                if (context) {
                    context->authLevel = AuthLevel::ADMIN;
                    
                    AsyncWebServerResponse* response = request->beginResponse(302);
                    response->addHeader("Location", "/");
                    response->addHeader("Set-Cookie", "session=" + context->sessionToken + "; Path=/; HttpOnly; Max-Age=1800");
                    request->send(response);
                    
                    Serial.println("WebInterface: Seed phrase confirmed and user logged in");
                    
                    // Load Lightning wallet if configured
                    if (wifiConnected) {
                        Serial.println("Loading Lightning wallet configuration on first-time setup");
                        lightningWallet.createWalletIfNeeded();
                        
                        Serial.println("Triggering balance update on first-time setup");
                        updateBalances();
                    }
                    return;
                }
            }
        }
        
        // Confirmation failed
        Serial.println("WebInterface: Seed phrase confirmation failed");
        String confirmPage = generateSeedConfirmPage();
        confirmPage.replace("{{error}}", "<div class='error'>❌ The words you entered don't match! Please try again carefully.</div>");
        request->send(400, "text/html; charset=utf-8", confirmPage);
    } else {
        request->send(200, "text/html; charset=utf-8", generateSeedConfirmPage());
    }
}

// HTML generators - simplified
String WebInterface::generateLandingPage() {
    // Get owner name (device name) from settings
    String ownerName = settings.getConfig().system.deviceName;
    if (ownerName == "Hodling Hog" || ownerName.isEmpty()) {
        ownerName = "Someone's"; // Default if no owner set
    } else {
        ownerName += "'s"; // Add possessive 
    }
    
    bool isSetup = settings.isSeedPhraseSet();
    
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>" + ownerName + " Hodling Hog</title>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { ";
    html += "font-family: 'Comic Sans MS', Arial, sans-serif; ";
    html += "margin: 0; ";
    html += "padding: 20px; ";
    html += "background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);";
    html += "min-height: 100vh;";
    html += "display: flex;";
    html += "align-items: center;";
    html += "justify-content: center;";
    html += "}";
    html += ".landing-container {";
    html += "max-width: 600px;";
    html += "margin: 0 auto;";
    html += "background: white;";
    html += "border-radius: 20px;";
    html += "box-shadow: 0 15px 35px rgba(0,0,0,0.1);";
    html += "padding: 3rem;";
    html += "text-align: center;";
    html += "border: 3px solid #ff6b9d;";
    html += "}";
    html += ".logo {";
    html += "font-size: 4rem;";
    html += "margin-bottom: 1rem;";
    html += "color: #333;";
    html += "}";
    html += ".title {";
    html += "color: #ff6b9d;";
    html += "font-size: 2.5rem;";
    html += "font-weight: bold;";
    html += "margin-bottom: 1rem;";
    html += "}";
    html += ".subtitle {";
    html += "color: #666;";
    html += "font-size: 1.3rem;";
    html += "margin-bottom: 2rem;";
    html += "line-height: 1.5;";
    html += "}";
    html += ".description {";
    html += "background: #e6f3ff;";
    html += "color: #0066cc;";
    html += "padding: 1.5rem;";
    html += "border-radius: 15px;";
    html += "margin: 2rem 0;";
    html += "border-left: 4px solid #0066cc;";
    html += "font-size: 1.1rem;";
    html += "line-height: 1.6;";
    html += "text-align: left;";
    html += "}";
    html += ".action-btn {";
    html += "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);";
    html += "color: white;";
    html += "padding: 1rem 2rem;";
    html += "border: none;";
    html += "border-radius: 25px;";
    html += "font-size: 1.2rem;";
    html += "cursor: pointer;";
    html += "transition: all 0.3s;";
    html += "margin: 0.5rem;";
    html += "font-family: inherit;";
    html += "font-weight: bold;";
    html += "text-decoration: none;";
    html += "display: inline-block;";
    html += "}";
    html += ".action-btn:hover {";
    html += "transform: translateY(-2px);";
    html += "box-shadow: 0 5px 15px rgba(0,0,0,0.2);";
    html += "}";
    html += ".action-btn.primary {";
    html += "background: linear-gradient(135deg, #28a745 0%, #20c997 100%);";
    html += "}";
    html += ".features {";
    html += "display: grid;";
    html += "grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));";
    html += "gap: 1rem;";
    html += "margin: 2rem 0;";
    html += "}";
    html += ".feature {";
    html += "background: #f8f9fa;";
    html += "padding: 1rem;";
    html += "border-radius: 10px;";
    html += "font-size: 0.9rem;";
    html += "}";
    html += ".feature-icon {";
    html += "font-size: 2rem;";
    html += "margin-bottom: 0.5rem;";
    html += "}";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='landing-container'>";
    html += "<div class='logo'>🐷⚡</div>";
    html += "<div class='title'>" + ownerName + " Hodling Hog</div>";
    html += "<div class='subtitle'>Your Personal Bitcoin Piggy Bank</div>";
    
    html += "<div class='description'>";
    html += "<strong>Welcome to Hodling Hog!</strong><br><br>";
    html += "This is a kid-friendly Bitcoin monitoring device that helps you track your Bitcoin savings. ";
    html += "Think of it as your digital piggy bank that shows how much Bitcoin you have in two places:<br><br>";
    html += "• <strong>Lightning Wallet</strong> - For small amounts and quick payments<br>";
    html += "• <strong>Cold Storage</strong> - For larger amounts kept extra safe<br><br>";
    html += "Your Hodling Hog keeps an eye on your Bitcoin 24/7 so you can watch your savings grow!";
    html += "</div>";
    
    html += "<div class='features'>";
    html += "<div class='feature'>";
    html += "<div class='feature-icon'>👀</div>";
    html += "<strong>Watch Only</strong><br>";
    html += "Safely monitor your Bitcoin without any risk";
    html += "</div>";
    html += "<div class='feature'>";
    html += "<div class='feature-icon'>⚡</div>";
    html += "<strong>Lightning Ready</strong><br>";
    html += "Track Lightning wallet balance and transactions";
    html += "</div>";
    html += "<div class='feature'>";
    html += "<div class='feature-icon'>❄️</div>";
    html += "<strong>Cold Storage</strong><br>";
    html += "Monitor your cold storage Bitcoin addresses";
    html += "</div>";
    html += "<div class='feature'>";
    html += "<div class='feature-icon'>📱</div>";
    html += "<strong>Easy Setup</strong><br>";
    html += "Simple web interface for all family members";
    html += "</div>";
    html += "</div>";
    
    if (isSetup) {
        html += "<p style='color: #666; margin: 1rem 0;'>This Hodling Hog has already been set up. Enter your secret words to access it.</p>";
        html += "<a href='/login' class='action-btn primary'>🔓 Login to My Piggy Bank</a>";
    } else {
        html += "<p style='color: #666; margin: 1rem 0;'>Let's get your Hodling Hog set up! We'll create some special words to keep it secure.</p>";
        html += "<a href='/generate-seed' class='action-btn primary'>🚀 Set Up My Hodling Hog</a>";
    }
    
    html += "</div>";
    html += "</body>";
    html += "</html>";
    return html;
}

String WebInterface::generateMainPage() {
    // Access the global wallet instances
    
    // Get current balances
    LightningBalance lnBalance = lightningWallet.getBalance();
    ColdBalance coldBalance = coldStorage.getBalance();
    
    // Convert satoshis to BTC for display
    float btcBalance = coldBalance.valid ? (float)coldBalance.total / 100000000.0 : 0.0;
    String btcString = coldBalance.valid ? String(btcBalance, 8) + " BTC" : "-- BTC";
    String lightningString = lnBalance.valid ? String(lnBalance.total) + " sats" : "-- sats";
    
    // Get Lightning address for display
    String lightningAddress = settings.getConfig().lightning.receiveAddress;
    bool hasLightningWallet = !lightningAddress.isEmpty();
    
    // Get owner name for title
    String ownerName = settings.getConfig().system.deviceName;
    if (ownerName == "Hodling Hog" || ownerName.isEmpty()) {
        ownerName = "My";
    } else {
        ownerName += "'s";
    }
    
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>" + ownerName + " Bitcoin Piggy Bank - Hodling Hog</title>";
    html += "<style>";
    html += "body { ";
    html += "font-family: 'Comic Sans MS', Arial, sans-serif; ";
    html += "margin: 0; ";
    html += "padding: 20px; ";
    html += "background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);";
    html += "min-height: 100vh;";
    html += "}";
    html += ".container {";
    html += "max-width: 800px;";
    html += "margin: 0 auto;";
    html += "background: white;";
    html += "border-radius: 20px;";
    html += "box-shadow: 0 10px 25px rgba(0,0,0,0.1);";
    html += "padding: 2rem;";
    html += "border: 3px solid #ff6b9d;";
    html += "}";
    html += ".header {";
    html += "text-align: center;";
    html += "margin-bottom: 2rem;";
    html += "border-bottom: 3px solid #f0f0f0;";
    html += "padding-bottom: 1rem;";
    html += "}";
    html += ".logo {";
    html += "font-size: 3.5rem;";
    html += "margin-bottom: 0.5rem;";
    html += "color: #333;";
    html += "}";
    html += ".subtitle {";
    html += "color: #ff6b9d;";
    html += "font-size: 1.3rem;";
    html += "font-weight: bold;";
    html += "}";
    html += ".nav {";
    html += "display: flex;";
    html += "justify-content: space-between;";
    html += "align-items: center;";
    html += "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);";
    html += "padding: 1rem;";
    html += "border-radius: 15px;";
    html += "margin-bottom: 2rem;";
    html += "color: white;";
    html += "}";
    html += ".nav-links {";
    html += "display: flex;";
    html += "gap: 1rem;";
    html += "}";
    html += ".nav-links a {";
    html += "color: white;";
    html += "text-decoration: none;";
    html += "padding: 0.5rem 1rem;";
    html += "border-radius: 10px;";
    html += "transition: background 0.3s;";
    html += "font-weight: bold;";
    html += "}";
    html += ".nav-links a:hover {";
    html += "background: rgba(255,255,255,0.2);";
    html += "}";
    html += ".auth-status {";
    html += "font-size: 0.9rem;";
    html += "color: #90ee90;";
    html += "font-weight: bold;";
    html += "}";
    html += ".logout-btn {";
    html += "background: #ff6b9d;";
    html += "color: white;";
    html += "padding: 0.5rem 1rem;";
    html += "border: none;";
    html += "border-radius: 10px;";
    html += "cursor: pointer;";
    html += "text-decoration: none;";
    html += "font-size: 0.9rem;";
    html += "font-weight: bold;";
    html += "transition: all 0.3s;";
    html += "}";
    html += ".logout-btn:hover {";
    html += "background: #e55a87;";
    html += "transform: translateY(-2px);";
    html += "}";
    html += ".status-grid {";
    html += "display: grid;";
    html += "grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));";
    html += "gap: 1rem;";
    html += "margin-bottom: 2rem;";
    html += "}";
    html += ".status-card {";
    html += "background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);";
    html += "padding: 1.5rem;";
    html += "border-radius: 15px;";
    html += "border-left: 5px solid #667eea;";
    html += "text-align: center;";
    html += "transition: transform 0.3s;";
    html += "}";
    html += ".status-card:hover {";
    html += "transform: translateY(-3px);";
    html += "}";
    html += ".status-title {";
    html += "font-weight: bold;";
    html += "color: #333;";
    html += "margin-bottom: 0.5rem;";
    html += "font-size: 1.1rem;";
    html += "}";
    html += ".status-value {";
    html += "font-size: 1.8rem;";
    html += "color: #667eea;";
    html += "font-weight: bold;";
    html += "}";
    html += ".welcome-message {";
    html += "background: linear-gradient(135deg, #ffeaa7 0%, #fab1a0 100%);";
    html += "padding: 1.5rem;";
    html += "border-radius: 15px;";
    html += "text-align: center;";
    html += "border: 3px solid #fdcb6e;";
    html += "}";
    html += ".welcome-title {";
    html += "font-size: 1.5rem;";
    html += "color: #333;";
    html += "margin-bottom: 1rem;";
    html += "font-weight: bold;";
    html += "}";
    html += ".welcome-text {";
    html += "color: #666;";
    html += "font-size: 1.1rem;";
    html += "line-height: 1.4;";
    html += "}";
    html += ".bitcoin-emoji {";
    html += "font-size: 2rem;";
    html += "margin: 0 0.5rem;";
    html += "}";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";
    html += "<div class='header'>";
    html += "<div class='logo'>" + (ownerName == "My" ? "Hodling Hog" : ownerName.substring(0, ownerName.length()-2) + "'s Hodling Hog") + "</div>";
    html += "<div class='subtitle'>Your Bitcoin Piggy Bank is Secure!</div>";
    html += "</div>";
    html += "<div class='nav'>";
    html += "<div class='nav-links'>";
    html += "<a href='/'>My Piggy Bank</a>";
    html += "<a href='/config'>Settings</a>";
    html += "</div>";
    html += "<div style='display: flex; align-items: center; gap: 1rem;'>";
    html += "<span class='auth-status'>Logged In!</span>";
    html += "<a href='/logout' class='logout-btn'>Logout</a>";
    html += "</div>";
    html += "</div>";
    html += "<div class='status-grid'>";
    html += "<div class='status-card'>";
    html += "<div class='status-title'>Lightning Sats</div>";
    html += "<div class='status-value'>" + lightningString + "</div>";
    html += "</div>";
    html += "<div class='status-card'>";
    html += "<div class='status-title'>Cold Storage</div>";
    html += "<div class='status-value'>" + btcString + "</div>";
    html += "</div>";
    html += "</div>";
    html += "<div class='welcome-message'>";
    html += "<div class='welcome-title'>Welcome to Your Bitcoin Adventure!</div>";
    html += "<div class='welcome-text'>";
    html += "Great job setting up your Hodling Hog! ";
    html += "This is your very own Bitcoin piggy bank where you can save and learn about digital money. ";
    html += "Use the menu above to explore and watch your savings grow!";
    if (hasLightningWallet) {
        html += "<br><br><strong>Lightning Address:</strong> " + lightningAddress;
        html += "<br>Send Lightning payments to this address to add sats to your wallet!";
    }
    html += "</div>";
    html += "</div>";
    html += "</div>";
    html += "</body>";
    html += "</html>";
    return html;
}

String WebInterface::generateConfigPage() {
    // Get current settings to populate form fields
    const HodlingHogConfig& config = settings.getConfig();
    
    String html = "<!DOCTYPE html>";
    html += "<html><head><title>Settings - Hodling Hog</title>";
    html += "<style>";
    html += "body{font-family:Arial;margin:0;padding:20px;background:#ffecd2;}";
    html += ".container{max-width:900px;margin:0 auto;background:white;border-radius:20px;padding:2rem;}";
    html += ".header{text-align:center;margin-bottom:2rem;border-bottom:3px solid #f0f0f0;padding-bottom:1rem;}";
    html += ".nav{background:#667eea;padding:1rem;border-radius:15px;margin-bottom:2rem;text-align:center;}";
    html += ".nav a{color:white;text-decoration:none;padding:0.5rem 1rem;margin:0 0.5rem;border-radius:10px;display:inline-block;font-weight:bold;}";
    html += ".nav a.active{background:rgba(255,255,255,0.3);}";
    html += ".section{background:#a8edea;margin-bottom:2rem;border-radius:15px;padding:2rem;}";
    html += ".section-title{font-size:1.5rem;color:#333;margin-bottom:1rem;font-weight:bold;}";
    html += ".current-value{background:#e8f5e8;padding:0.5rem;border-radius:5px;margin-bottom:1rem;font-size:0.9rem;color:#2d5f2d;}";
    html += ".form-group{margin-bottom:1.5rem;}";
    html += ".form-label{display:block;font-weight:bold;color:#333;margin-bottom:0.5rem;}";
    html += ".form-input{width:100%;padding:0.75rem;border:2px solid #ddd;border-radius:10px;font-size:1rem;box-sizing:border-box;}";
    html += ".save-btn{background:#667eea;color:white;padding:0.75rem 2rem;border:none;border-radius:25px;font-size:1.1rem;cursor:pointer;font-weight:bold;margin-top:1rem;}";
    html += ".danger-zone{background:#ffebee;border:2px solid #f44336;margin-top:2rem;}";
    html += ".danger-btn{background:#f44336;color:white;padding:0.75rem 2rem;border:none;border-radius:25px;font-size:1.1rem;cursor:pointer;font-weight:bold;margin-top:1rem;}";
    html += ".danger-btn:hover{background:#d32f2f;}";
    html += ".warning-text{color:#d32f2f;font-weight:bold;margin-bottom:1rem;}";
    html += ".warning-text ul{margin:0.5rem 0;padding-left:1.5rem;}";
    html += ".warning-text li{margin:0.25rem 0;}";
    html += ".info-box{background:#e3f2fd;border:1px solid #2196f3;padding:1rem;border-radius:8px;margin-bottom:1rem;}";
    html += ".info-box strong{color:#1976d2;}";
    html += ".grid-2{display:grid;grid-template-columns:1fr 1fr;gap:1rem;}";
    html += ".success-msg{background:#d4edda;color:#155724;padding:1rem;border-radius:10px;margin-bottom:2rem;border-left:4px solid #28a745;}";
    html += ".error-msg{background:#f8d7da;color:#721c24;padding:1rem;border-radius:10px;margin-bottom:2rem;border-left:4px solid #dc3545;}";
    html += "</style></head><body>";
    html += "<div class='container'>";
    html += "<div class='header'><h1>Hodling Hog</h1><p>Settings & Configuration</p></div>";
    html += "<div class='nav'>";
    html += "<a href='/'>Home</a>";
    html += "<a href='/config' class='active'>Settings</a>";
    html += "</div>";
    
    // Add success/error messages based on query parameters
    html += "<script>";
    html += "var urlParams = new URLSearchParams(window.location.search);";
    html += "var saved = urlParams.get('saved');";
    html += "var error = urlParams.get('error');";
    html += "if(saved) {";
    html += "  var msg = '';";
    html += "  if(saved === 'wifi') msg = 'WiFi settings saved successfully!';";
    html += "  else if(saved === 'lightning') msg = 'Lightning wallet settings saved successfully!';";
    html += "  else if(saved === 'coldstorage') msg = 'Cold storage address saved successfully!';";
    html += "  else if(saved === 'system') msg = 'System settings saved successfully!';";
    html += "  if(msg) document.write('<div class=\"success-msg\">' + msg + '</div>');";
    html += "}";
    html += "if(error) {";
    html += "  var msg = '';";
    html += "  if(error === 'wifi') msg = 'Error saving WiFi settings. Please try again.';";
    html += "  else if(error === 'lightning') msg = 'Error saving Lightning settings. Please check your API token.';";
    html += "  else if(error === 'coldstorage') msg = 'Error saving cold storage address. Please check the address format.';";
    html += "  else if(error === 'system') msg = 'Error saving system settings. Please check the sleep timeout value (1-60 minutes).';";
    html += "  if(msg) document.write('<div class=\"error-msg\">' + msg + '</div>');";
    html += "}";
    html += "</script>";
    
    html += "<div class='section'>";
    html += "<div class='section-title'>WiFi Settings</div>";
    // Show current WiFi settings if available
    if (!config.wifi.ssid.isEmpty()) {
        html += "<div class='current-value'>Current WiFi: " + config.wifi.ssid + "</div>";
    }
    html += "<form method='POST' action='/api/config/wifi'>";
    html += "<div class='grid-2'>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Network Name (SSID)</label>";
    html += "<input type='text' name='ssid' class='form-input' placeholder='YourWiFiNetwork' value='" + config.wifi.ssid + "' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Password</label>";
    html += "<input type='password' name='password' class='form-input' placeholder='WiFi Password' value='" + config.wifi.password + "'>";
    html += "</div>";
    html += "</div>";
    html += "<button type='submit' class='save-btn'>Save WiFi Settings</button>";
    html += "</form>";
    html += "</div>";
    
    html += "<div class='section'>";
    html += "<div class='section-title'>⚡ Lightning Wallet Settings (Wallet of Satoshi)</div>";
    
    // Instructions for getting WoS credentials
    html += "<div class='info-box'>";
    html += "<strong>📱 How to get Wallet of Satoshi API credentials:</strong><br>";
    html += "1. Download the Wallet of Satoshi app<br>";
    html += "2. Create an account and verify your email<br>";
    html += "3. Go to Settings → Developer → API Keys<br>";
    html += "4. Generate new API credentials<br>";
    html += "5. Copy the API Token and API Secret below";
    html += "</div>";
    
    // Show current Lightning settings if available
    if (!config.lightning.apiToken.isEmpty()) {
        html += "<div class='current-value'>✅ API Token: " + config.lightning.apiToken.substring(0, 8) + "...*** (configured)</div>";
        if (!config.lightning.receiveAddress.isEmpty()) {
            html += "<div class='current-value'>📧 Lightning Address: " + config.lightning.receiveAddress + "</div>";
        }
    } else {
        html += "<div class='warning-text'>⚠️ No Lightning wallet configured. Add your WoS credentials below.</div>";
    }
    
    html += "<form method='POST' action='/api/config/lightning'>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WoS API Token *</label>";
    html += "<input type='password' name='api_token' class='form-input' placeholder='Your WoS API Token' value='" + config.lightning.apiToken + "'>";
    html += "<small style='color:#666;'>Get this from Wallet of Satoshi app → Settings → Developer → API Keys</small>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>WoS API Secret *</label>";
    html += "<input type='password' name='api_secret' class='form-input' placeholder='Your WoS API Secret' value='" + config.lightning.apiSecret + "'>";
    html += "<small style='color:#666;'>Keep this secret safe - it's used for signing transactions</small>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Lightning Address</label>";
    html += "<input type='email' name='lightning_address' class='form-input' placeholder='yourname@walletofsatoshi.com' value='" + config.lightning.receiveAddress + "'>";
    html += "<small style='color:#666;'>Your Lightning address for receiving payments (optional)</small>";
    html += "</div>";
    html += "<button type='submit' class='save-btn'>Save Lightning Settings</button>";
    html += "</form>";
    html += "</div>";
    
    html += "<div class='section'>";
    html += "<div class='section-title'>Cold Storage Settings</div>";
    // Show current cold storage address if available
    if (!config.coldStorage.watchAddress.isEmpty()) {
        html += "<div class='current-value'>Current Address: " + config.coldStorage.watchAddress + "</div>";
    }
    html += "<form method='POST' action='/api/config/coldstorage'>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Bitcoin Address</label>";
    html += "<input type='text' name='address' class='form-input' placeholder='bc1q... (your Bitcoin address)' value='" + config.coldStorage.watchAddress + "' required>";
    html += "</div>";
    html += "<button type='submit' class='save-btn'>Save Cold Storage Settings</button>";
    html += "</form>";
    html += "</div>";

    // System Settings Section
    html += "<div class='section'>";
    html += "<div class='section-title'>System Settings</div>";
    // Show current device name (owner name)
    String currentDeviceName = config.system.deviceName;
    if (currentDeviceName == "Hodling Hog" || currentDeviceName.isEmpty()) {
        html += "<div class='current-value'>Current Owner: Not set (showing as default)</div>";
    } else {
        html += "<div class='current-value'>Current Owner: " + currentDeviceName + "</div>";
    }
    // Show current sleep timeout
    uint32_t currentSleepTimeoutMinutes = config.power.sleepTimeout / 60000; // Convert ms to minutes
    html += "<div class='current-value'>Current Sleep Timeout: " + String(currentSleepTimeoutMinutes) + " minutes</div>";
    html += "<form method='POST' action='/api/config/system'>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Owner Name:</label>";
    html += "<input type='text' name='ownerName' class='form-input' placeholder='Enter your name (e.g., Alice)' value='" + (currentDeviceName == "Hodling Hog" ? "" : currentDeviceName) + "' maxlength='20'>";
    html += "<small style='color:#666;'>This will show as \"YourName's Hodling Hog\" on the device</small>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label class='form-label'>Sleep Timeout (minutes):</label>";
    html += "<input type='number' name='sleepTimeout' class='form-input' placeholder='3' value='" + String(currentSleepTimeoutMinutes) + "' min='1' max='60' required>";
    html += "<small style='color:#666;'>Device will sleep after this many minutes of inactivity</small>";
    html += "</div>";
    html += "<button type='submit' class='save-btn'>Save System Settings</button>";
    html += "</form>";
    html += "</div>";

    // Danger Zone Section
    html += "<div class='section danger-zone'>";
    html += "<div class='section-title'>⚠️ Danger Zone</div>";
    html += "<div class='warning-text'>";
    html += "This action cannot be undone! Factory reset will permanently erase:";
    html += "<ul>";
    html += "<li>🔑 Seed phrase and login credentials</li>";
    html += "<li>⚡ Lightning wallet data</li>";
    html += "<li>❄️ Cold storage settings</li>";
    html += "<li>📶 WiFi configuration</li>";
    html += "<li>⚙️ All system settings</li>";
    html += "</ul>";
    html += "</div>";
    html += "<button type='button' class='danger-btn' onclick='confirmFactoryReset()'>🗑️ Factory Reset Device</button>";
    html += "</div>";
    
    html += "</div>";
    
    // Add JavaScript for factory reset confirmation
    html += "<script>";
    html += "function confirmFactoryReset() {";
    html += "  if(confirm('⚠️ DANGER: This will permanently erase ALL data including your seed phrase!\\n\\nAre you absolutely sure you want to factory reset?')) {";
    html += "    if(confirm('⚠️ FINAL WARNING: Your Lightning wallet and all settings will be lost forever!\\n\\nContinue with factory reset?')) {";
    html += "      window.location.href = '/api/factory-reset';";
    html += "    }";
    html += "  }";
    html += "}";
    html += "</script>";
    
    html += "</body></html>";
    return html;
}

// Commented out - wallet functionality removed for passive mode
/*
String WebInterface::generateWalletPage() {
    updateWebActivity(); // Reset sleep timer on web activity
    
    // Get current balances
    LightningBalance lnBalance = lightningWallet.getBalance();
    ColdBalance coldBalance = coldStorage.getBalance();
    
    // Convert satoshis to BTC for display
    float btcBalance = coldBalance.valid ? (float)coldBalance.total / 100000000.0 : 0.0;
    String btcString = coldBalance.valid ? String(btcBalance, 8) + " BTC" : "-- BTC";
    String lightningString = lnBalance.valid ? String(lnBalance.total) + " sats" : "-- sats";
    
    // Get Lightning address
    String lightningAddress = settings.getConfig().lightning.receiveAddress;
    bool hasLightningWallet = !lightningAddress.isEmpty();
    
    // Get cold storage address
    String coldAddress = settings.getConfig().coldStorage.watchAddress;
    bool hasColdStorage = !coldAddress.isEmpty();
    
    
    return R"(<!DOCTYPE html>
<html>
<head>
    <title>Wallet - Hodling Hog</title>
    <meta charset="utf-8">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: linear-gradient(135deg, #ffecd2 0%, #fcb69f 100%);
            min-height: 100vh;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.1);
            padding: 2rem;
            border: 3px solid #ff6b9d;
        }
        .header {
            text-align: center;
            margin-bottom: 2rem;
            border-bottom: 3px solid #f0f0f0;
            padding-bottom: 1rem;
        }
        .logo {
            font-size: 3rem;
            color: #333;
        }
        .subtitle {
            color: #ff6b9d;
            font-size: 1.3rem;
            font-weight: bold;
        }
        .nav {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 1rem;
            border-radius: 15px;
            margin-bottom: 2rem;
            text-align: center;
        }
        .nav a {
            color: white;
            text-decoration: none;
            padding: 0.5rem 1rem;
            margin: 0 0.5rem;
            border-radius: 10px;
            display: inline-block;
            font-weight: bold;
        }
        .nav a.active {
            background: rgba(255,255,255,0.3);
        }
        .balance-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 2rem;
            margin-bottom: 2rem;
        }
        .balance-card {
            background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);
            padding: 2rem;
            border-radius: 15px;
            border-left: 5px solid #667eea;
            text-align: center;
        }
        .balance-title {
            font-size: 1.5rem;
            color: #333;
            margin-bottom: 1rem;
            font-weight: bold;
        }
        .balance-amount {
            font-size: 2.5rem;
            color: #667eea;
            font-weight: bold;
            margin-bottom: 0.5rem;
        }
        .balance-status {
            font-size: 0.9rem;
            color: #666;
        }
        .section {
            background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);
            margin-bottom: 2rem;
            border-radius: 15px;
            border-left: 5px solid #667eea;
            padding: 2rem;
        }
        .section-title {
            font-size: 1.5rem;
            color: #333;
            margin-bottom: 1rem;
            font-weight: bold;
        }
        .action-btn {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 1rem 1.5rem;
            border: none;
            border-radius: 15px;
            font-size: 1rem;
            cursor: pointer;
            font-weight: bold;
            margin: 0.5rem;
            display: inline-block;
        }
        .action-btn.secondary {
            background: linear-gradient(135deg, #ffeaa7 0%, #fab1a0 100%);
            color: #333;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">Hodling Hog</div>
            <div class="subtitle">Wallet Management</div>
        </div>
        
        <div class="nav">
            <a href="/">Home</a>
            <a href="/config">Settings</a>
        </div>

        <div class="balance-grid">
            <div class="balance-card">
                <div class="balance-title">Lightning Balance</div>
                <div class="balance-amount">1,234 sats</div>
                <div class="balance-status">Connected & Synced</div>
            </div>
            <div class="balance-card">
                <div class="balance-title">Cold Storage</div>
                <div class="balance-amount">0.05 BTC</div>
                <div class="balance-status">Monitoring Address</div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">Lightning Wallet Actions</div>
            <button class="action-btn">Receive Payment</button>
            <button class="action-btn">Send Payment</button>
            <button class="action-btn secondary">View History</button>
            <button class="action-btn secondary">Refresh Balance</button>
        </div>

        <div class="section">
            <div class="section-title">Cold Storage Monitoring</div>
            <button class="action-btn secondary">Check Balance</button>
            <button class="action-btn secondary">Show Address</button>
            <button class="action-btn secondary">Export Data</button>
        </div>
    html += "</div></body></html>";
    return html;
}
*/

// Commented out - transfer functionality removed for passive mode
/*
String WebInterface::generateTransferPage() {
    return "<html><body><h1>Transfer</h1></body></html>";
}
*/

String WebInterface::generateSystemPage() {
    return "<!DOCTYPE html><html><head><title>System</title></head><body><h1>System Page Not Available</h1><a href='/'>Home</a> | <a href='/config'>Settings</a></body></html>";
}

String WebInterface::generateCaptivePortalPage() {
    return generateMainPage();
}

String WebInterface::generateLoginPage() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Login to Your Piggy Bank - Hodling Hog</title>
    <style>
        body { 
            font-family: 'Comic Sans MS', Arial, sans-serif; 
            margin: 0; 
            padding: 0; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .login-container {
            background: white;
            padding: 2rem;
            border-radius: 15px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.2);
            max-width: 400px;
            width: 90%;
            border: 3px solid #667eea;
        }
        .logo {
            text-align: center;
            font-size: 2.5rem;
            margin-bottom: 1rem;
            color: #333;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 2rem;
            font-size: 1.1rem;
        }
        .form-group {
            margin-bottom: 1rem;
        }
        label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: bold;
            color: #333;
            font-size: 1.1rem;
        }
        .seed-input {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 1.1rem;
            font-family: 'Courier New', monospace;
            text-align: center;
            letter-spacing: 1px;
        }
        .seed-input:focus {
            border-color: #667eea;
            outline: none;
            box-shadow: 0 0 10px rgba(102, 126, 234, 0.3);
        }
        .login-btn {
            width: 100%;
            padding: 0.75rem;
            background: #667eea;
            color: white;
            border: none;
            border-radius: 25px;
            font-size: 1.1rem;
            cursor: pointer;
            transition: all 0.3s;
            font-family: inherit;
            font-weight: bold;
        }
        .login-btn:hover {
            background: #5a6fd8;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .error {
            background: #ffe6e6;
            color: #d00;
            padding: 0.75rem;
            border-radius: 5px;
            margin-bottom: 1rem;
            border-left: 4px solid #d00;
        }
        .info {
            background: #e6f3ff;
            color: #0066cc;
            padding: 0.75rem;
            border-radius: 5px;
            margin-bottom: 1rem;
            border-left: 4px solid #0066cc;
            font-size: 0.9rem;
        }
        .word-count {
            font-size: 0.8rem;
            color: #666;
            text-align: right;
            margin-top: 0.25rem;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="logo">🐷⚡ Hodling Hog</div>
        <div class="subtitle">Welcome back! Open your piggy bank</div>
        
        {{error}}
        
        <div class="info">
            Enter the 4 special words you wrote down to access your Bitcoin piggy bank! 🔐
        </div>
        
        <form method="POST" action="/login">
            <div class="form-group">
                <label for="seedphrase">Your 4 Secret Words:</label>
                <input 
                    type="text" 
                    id="seedphrase" 
                    name="seedphrase" 
                    class="seed-input"
                    placeholder="word1 word2 word3 word4"
                    required
                    autocomplete="off"
                    autocapitalize="none"
                    autocorrect="off"
                    spellcheck="false"
                />
                <div class="word-count" id="wordCount">0 words</div>
            </div>
            
            <button type="submit" class="login-btn">🔓 Open My Piggy Bank!</button>
        </form>
    </div>
    
    <script>
        document.getElementById('seedphrase').addEventListener('input', function() {
            const words = this.value.trim().split(/\s+/).filter(word => word.length > 0);
            document.getElementById('wordCount').textContent = words.length + ' words';
            
            if (words.length === 4) {
                document.getElementById('wordCount').style.color = '#0a8';
                document.querySelector('.login-btn').style.background = '#667eea';
            } else {
                document.getElementById('wordCount').style.color = '#666';
                document.querySelector('.login-btn').style.background = '#6c757d';
            }
        });
        
        // Clear any error placeholder if no error
        if (document.querySelector('.error') === null) {
            document.body.innerHTML = document.body.innerHTML.replace('{{error}}', '');
        }
    </script>
</body>
</html>
)";
}

String WebInterface::generateSetupPage() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Hodling Hog - First Time Setup</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 0; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .setup-container {
            background: white;
            padding: 2rem;
            border-radius: 10px;
            box-shadow: 0 10px 25px rgba(0,0,0,0.2);
            max-width: 500px;
            width: 90%;
        }
        .logo {
            text-align: center;
            font-size: 2.5rem;
            margin-bottom: 1rem;
            color: #333;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 2rem;
        }
        .form-group {
            margin-bottom: 1rem;
        }
        label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: bold;
            color: #333;
        }
        .seed-input {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 1rem;
            min-height: 100px;
            resize: vertical;
            font-family: monospace;
        }
        .seed-input:focus {
            border-color: #667eea;
            outline: none;
        }
        .setup-btn {
            width: 100%;
            padding: 0.75rem;
            background: #28a745;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 1rem;
            cursor: pointer;
            transition: background 0.3s;
        }
        .setup-btn:hover {
            background: #218838;
        }
        .error {
            background: #ffe6e6;
            color: #d00;
            padding: 0.75rem;
            border-radius: 5px;
            margin-bottom: 1rem;
            border-left: 4px solid #d00;
        }
        .warning {
            background: #fff3cd;
            color: #856404;
            padding: 0.75rem;
            border-radius: 5px;
            margin-bottom: 1rem;
            border-left: 4px solid #ffc107;
            font-size: 0.9rem;
        }
        .info {
            background: #e6f3ff;
            color: #0066cc;
            padding: 0.75rem;
            border-radius: 5px;
            margin-bottom: 1rem;
            border-left: 4px solid #0066cc;
            font-size: 0.9rem;
        }
        .word-count {
            font-size: 0.8rem;
            color: #666;
            text-align: right;
            margin-top: 0.25rem;
        }
        .example {
            background: #f8f9fa;
            padding: 0.5rem;
            border-radius: 3px;
            font-family: monospace;
            font-size: 0.8rem;
            margin-top: 0.5rem;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="setup-container">
        <div class="logo">🐷⚡ Hodling Hog</div>
        <div class="subtitle">First Time Setup</div>
        
        {{error}}
        
        <div class="warning">
            <strong>⚠️ Important Security Information</strong><br>
            This seed phrase will be used to protect access to your Hodling Hog device. 
            Store it securely and never share it with anyone!
        </div>
        
        <div class="info">
            <strong>Setup Instructions:</strong><br>
            1. Generate a new 12-word seed phrase using a trusted wallet app<br>
            2. Write it down on paper and store it safely<br>
            3. Enter it below to secure your device<br>
            4. You'll need this phrase to access the web interface
        </div>
        
        <form method="POST" action="/setup">
            <div class="form-group">
                <label for="seedphrase">Enter your 12-word seed phrase:</label>
                <textarea 
                    id="seedphrase" 
                    name="seedphrase" 
                    class="seed-input"
                    placeholder="Enter 12 words separated by spaces..."
                    required
                    autocomplete="off"
                    autocapitalize="none"
                    autocorrect="off"
                    spellcheck="false"
                ></textarea>
                <div class="word-count" id="wordCount">0 words</div>
                <div class="example">
                    Example: abandon ability able about above absent absorb abstract absurd abuse access accident
                </div>
            </div>
            
            <button type="submit" class="setup-btn">🔐 Secure My Device</button>
        </form>
    </div>
    
    <script>
        document.getElementById('seedphrase').addEventListener('input', function() {
            const words = this.value.trim().split(/\s+/).filter(word => word.length > 0);
            document.getElementById('wordCount').textContent = words.length + ' words';
            
            if (words.length === 12) {
                document.getElementById('wordCount').style.color = '#28a745';
                document.querySelector('.setup-btn').style.background = '#28a745';
            } else {
                document.getElementById('wordCount').style.color = '#666';
                document.querySelector('.setup-btn').style.background = '#6c757d';
            }
        });
        
        // Clear any error placeholder if no error
        if (document.querySelector('.error') === null) {
            document.body.innerHTML = document.body.innerHTML.replace('{{error}}', '');
        }
    </script>
</body>
</html>
)";
}

String WebInterface::generateSeedDisplayPage(const String& seedPhrase) {
    String page = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Your Secret Words - Hodling Hog</title>
    <style>
        body { 
            font-family: 'Comic Sans MS', Arial, sans-serif; 
            margin: 0; 
            padding: 0; 
            background: linear-gradient(135deg, #ff9a9e 0%, #fecfef 50%, #fecfef 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .seed-container {
            background: white;
            padding: 2rem;
            border-radius: 20px;
            box-shadow: 0 15px 35px rgba(0,0,0,0.1);
            max-width: 500px;
            width: 90%;
            text-align: center;
            border: 3px solid #ff6b9d;
        }
        .logo {
            font-size: 3rem;
            margin-bottom: 1rem;
            color: #333;
        }
        .title {
            color: #ff6b9d;
            font-size: 1.8rem;
            font-weight: bold;
            margin-bottom: 1rem;
        }
        .subtitle {
            color: #666;
            margin-bottom: 2rem;
            font-size: 1.1rem;
        }
        .seed-words {
            background: #f8f9ff;
            padding: 2rem;
            border-radius: 15px;
            margin: 2rem 0;
            border: 2px dashed #ff6b9d;
            font-family: 'Courier New', monospace;
            font-size: 1.8rem;
            font-weight: bold;
            color: #333;
            letter-spacing: 2px;
            line-height: 1.6;
        }
        .warning {
            background: #fff3cd;
            color: #856404;
            padding: 1rem;
            border-radius: 10px;
            margin: 1rem 0;
            border-left: 4px solid #ffc107;
            font-size: 0.95rem;
            text-align: left;
        }
        .instructions {
            background: #e6f3ff;
            color: #0066cc;
            padding: 1rem;
            border-radius: 10px;
            margin: 1rem 0;
            border-left: 4px solid #0066cc;
            font-size: 0.95rem;
            text-align: left;
        }
        .continue-btn {
            background: #28a745;
            color: white;
            padding: 1rem 2rem;
            border: none;
            border-radius: 25px;
            font-size: 1.2rem;
            cursor: pointer;
            transition: all 0.3s;
            margin-top: 1rem;
            font-family: inherit;
            font-weight: bold;
        }
        .continue-btn:hover {
            background: #218838;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .step-indicator {
            background: #ff6b9d;
            color: white;
            padding: 0.5rem 1rem;
            border-radius: 20px;
            font-size: 0.9rem;
            margin-bottom: 1rem;
            display: inline-block;
        }
    </style>
</head>
<body>
    <div class="seed-container">
        <div class="step-indicator">📝 Step 1 of 2: Write Down Your Words</div>
        <div class="logo">🐷⚡ Hodling Hog</div>
        <div class="title">Your Secret Words!</div>
        <div class="subtitle">These 4 special words will protect your Bitcoin piggy bank</div>
        
                 <div class="seed-words">)" + seedPhrase + R"(</div>
        
        <div class="warning">
            <strong>⚠️ Very Important!</strong><br>
            Write these 4 words on a piece of paper RIGHT NOW! 📝<br>
            Keep the paper safe - you'll need these words to open your piggy bank!
        </div>
        
        <div class="instructions">
            <strong>📚 What to do:</strong><br>
            1. Get a piece of paper and a pencil ✏️<br>
            2. Write down all 4 words exactly as shown<br>
            3. Keep your paper somewhere safe (like with your other important papers)<br>
            4. Click continue when you're done writing
        </div>
        
        <a href="/confirm-seed">
            <button class="continue-btn">✅ I wrote them down!</button>
        </a>
    </div>
</body>
</html>
)";
    return page;
}

String WebInterface::generateSeedConfirmPage() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Confirm Your Words - Hodling Hog</title>
    <style>
        body { 
            font-family: 'Comic Sans MS', Arial, sans-serif; 
            margin: 0; 
            padding: 0; 
            background: linear-gradient(135deg, #a8edea 0%, #fed6e3 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .confirm-container {
            background: white;
            padding: 2rem;
            border-radius: 20px;
            box-shadow: 0 15px 35px rgba(0,0,0,0.1);
            max-width: 500px;
            width: 90%;
            text-align: center;
            border: 3px solid #6fb3d9;
        }
        .logo {
            font-size: 3rem;
            margin-bottom: 1rem;
            color: #333;
        }
        .title {
            color: #6fb3d9;
            font-size: 1.8rem;
            font-weight: bold;
            margin-bottom: 1rem;
        }
        .subtitle {
            color: #666;
            margin-bottom: 2rem;
            font-size: 1.1rem;
        }
        .form-group {
            margin-bottom: 1.5rem;
            text-align: left;
        }
        label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: bold;
            color: #333;
            font-size: 1.1rem;
        }
        .seed-input {
            width: 100%;
            padding: 1rem;
            border: 3px solid #ddd;
            border-radius: 15px;
            font-size: 1.2rem;
            font-family: 'Courier New', monospace;
            text-align: center;
            letter-spacing: 2px;
        }
        .seed-input:focus {
            border-color: #6fb3d9;
            outline: none;
            box-shadow: 0 0 10px rgba(111, 179, 217, 0.3);
        }
        .confirm-btn {
            background: #28a745;
            color: white;
            padding: 1rem 2rem;
            border: none;
            border-radius: 25px;
            font-size: 1.2rem;
            cursor: pointer;
            transition: all 0.3s;
            margin-top: 1rem;
            font-family: inherit;
            font-weight: bold;
            width: 100%;
        }
        .confirm-btn:hover {
            background: #218838;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(0,0,0,0.2);
        }
        .error {
            background: #ffe6e6;
            color: #d00;
            padding: 1rem;
            border-radius: 10px;
            margin-bottom: 1rem;
            border-left: 4px solid #d00;
            text-align: left;
        }
        .instructions {
            background: #e6f3ff;
            color: #0066cc;
            padding: 1rem;
            border-radius: 10px;
            margin: 1rem 0;
            border-left: 4px solid #0066cc;
            font-size: 0.95rem;
            text-align: left;
        }
        .step-indicator {
            background: #6fb3d9;
            color: white;
            padding: 0.5rem 1rem;
            border-radius: 20px;
            font-size: 0.9rem;
            margin-bottom: 1rem;
            display: inline-block;
        }
        .word-count {
            font-size: 0.9rem;
            color: #666;
            text-align: right;
            margin-top: 0.5rem;
        }
    </style>
</head>
<body>
    <div class="confirm-container">
        <div class="step-indicator">✅ Step 2 of 2: Confirm Your Words</div>
        <div class="logo">🐷⚡ Hodling Hog</div>
        <div class="title">Now Type Your Words</div>
        <div class="subtitle">Show me you wrote them down correctly!</div>
        
        {{error}}
        
        <div class="instructions">
            <strong>🔍 Type the 4 words you wrote down:</strong><br>
            • Type them exactly as they appeared<br>
            • Separate each word with a space<br>
            • Check your spelling carefully!
        </div>
        
        <form method="POST" action="/confirm-seed">
            <div class="form-group">
                <label for="seedphrase">Enter your 4 words:</label>
                <input 
                    type="text" 
                    id="seedphrase" 
                    name="seedphrase" 
                    class="seed-input"
                    placeholder="word1 word2 word3 word4"
                    required
                    autocomplete="off"
                    autocapitalize="none"
                    autocorrect="off"
                    spellcheck="false"
                />
                <div class="word-count" id="wordCount">0 words</div>
            </div>
            
            <button type="submit" class="confirm-btn">🔐 Confirm & Secure My Piggy Bank!</button>
        </form>
    </div>
    
    <script>
        document.getElementById('seedphrase').addEventListener('input', function() {
            const words = this.value.trim().split(/\s+/).filter(word => word.length > 0);
            document.getElementById('wordCount').textContent = words.length + ' words';
            
            if (words.length === 4) {
                document.getElementById('wordCount').style.color = '#28a745';
                document.querySelector('.confirm-btn').style.background = '#28a745';
            } else {
                document.getElementById('wordCount').style.color = '#666';
                document.querySelector('.confirm-btn').style.background = '#6c757d';
            }
        });
        
        // Clear any error placeholder if no error
        if (document.querySelector('.error') === null) {
            document.body.innerHTML = document.body.innerHTML.replace('{{error}}', '');
        }
    </script>
</body>
</html>
)";
}

String WebInterface::getCSS() {
    return "body{font-family:Arial,sans-serif;margin:20px;}";
}

String WebInterface::getJavaScript() {
    return "console.log('Hodling Hog Web Interface');";
}

String WebInterface::getBootstrapCSS() {
    return "";
}

String WebInterface::getJQuery() {
    return "";
}

// Session management stubs
void WebInterface::cleanupSessions() {
    // Clean up expired sessions
    unsigned long currentTime = millis();
    
    for (auto it = activeSessions.begin(); it != activeSessions.end();) {
        if (currentTime - it->second.sessionStart > SESSION_TIMEOUT) {
            Serial.printf("WebInterface: Cleaning up expired session: %s\n", it->first.c_str());
            it = activeSessions.erase(it);
        } else {
            ++it;
        }
    }
}

WebContext* WebInterface::getSession(const String& token) {
    auto it = activeSessions.find(token);
    return (it != activeSessions.end()) ? &it->second : nullptr;
}

WebContext* WebInterface::createSession(const String& clientIP, const String& userAgent) {
    String token = generateSessionToken();
    WebContext context;
    context.clientIP = clientIP;
    context.userAgent = userAgent;
    context.authLevel = AuthLevel::NONE;
    context.sessionStart = millis();
    context.sessionToken = token;
    
    activeSessions[token] = context;
    Serial.printf("WebInterface: Created session %s for %s\n", token.c_str(), clientIP.c_str());
    return &activeSessions[token];
}

void WebInterface::updateSessionActivity(const String& token) {
    auto it = activeSessions.find(token);
    if (it != activeSessions.end()) {
        it->second.sessionStart = millis(); // Update to current time
    }
}

// All other methods are stubs
void WebInterface::sendJsonResponse(AsyncWebServerRequest* request, const JsonDocument& doc, int httpCode) {}
void WebInterface::sendErrorResponse(AsyncWebServerRequest* request, const String& error, int httpCode) {
    Serial.printf("WebInterface: Sending error response - %d: %s\n", httpCode, error.c_str());
    String jsonError = "{\"error\":\"" + error + "\",\"code\":" + String(httpCode) + "}";
    request->send(httpCode, "application/json; charset=utf-8", jsonError);
}
void WebInterface::sendSuccessResponse(AsyncWebServerRequest* request, const String& message) {}
JsonDocument WebInterface::createStatusJson() { return JsonDocument(); }
JsonDocument WebInterface::createBalanceJson() { return JsonDocument(); }
JsonDocument WebInterface::createConfigJson() { return JsonDocument(); }
bool WebInterface::checkRateLimit(const String& clientIP) { return true; }
void WebInterface::logSecurityEvent(const String& event, const String& clientIP) {}
String WebInterface::hashPassword(const String& password) { return password; }
bool WebInterface::verifyPassword(const String& password, const String& hash) { return true; }
void WebInterface::serveStaticFile(AsyncWebServerRequest* request, const String& filename) {}
String WebInterface::getContentType(const String& filename) { return "text/html"; }
bool WebInterface::fileExists(const String& path) { return false; }
void WebInterface::handleWebError(const String& error) {}
void WebInterface::logWebAccess(AsyncWebServerRequest* request, int responseCode) {}
String WebInterface::urlDecode(const String& str) { return str; }
String WebInterface::urlEncode(const String& str) { return str; }
String WebInterface::getClientIP(AsyncWebServerRequest* request) { 
    // Check for forwarded IP first
    if (request->hasHeader("X-Forwarded-For")) {
        return request->header("X-Forwarded-For");
    }
    if (request->hasHeader("X-Real-IP")) {
        return request->header("X-Real-IP");
    }
    
    // Get direct client IP
    return request->client()->remoteIP().toString();
}
String WebInterface::formatFileSize(size_t bytes) { return String(bytes) + " bytes"; }
String WebInterface::formatUptime() { return String(millis() / 1000) + "s"; }
bool WebInterface::isValidJSON(const String& json) { return true; }
void WebInterface::startMDNS() {}
void WebInterface::stopMDNS() {}
bool WebInterface::checkInternetConnection() { return true; }
String WebInterface::getWiFiStatusString() { return "Connected"; }
bool WebInterface::isCaptivePortalRequest(AsyncWebServerRequest* request) { return false; }
void WebInterface::redirectToCaptivePortal(AsyncWebServerRequest* request) {}
void WebInterface::handleDNSRedirect() {}

// API and config handlers - all stubs
void WebInterface::apiGetStatus(AsyncWebServerRequest* request) {}
void WebInterface::apiGetBalances(AsyncWebServerRequest* request) {}
void WebInterface::apiUpdateBalances(AsyncWebServerRequest* request) {}
void WebInterface::apiGetConfig(AsyncWebServerRequest* request) {}
void WebInterface::apiSetConfig(AsyncWebServerRequest* request) {}
void WebInterface::apiGetTransactions(AsyncWebServerRequest* request) {}
void WebInterface::apiCreateInvoice(AsyncWebServerRequest* request) {}
void WebInterface::apiSendPayment(AsyncWebServerRequest* request) {}
void WebInterface::apiTransferFunds(AsyncWebServerRequest* request) {}
void WebInterface::apiSignTransaction(AsyncWebServerRequest* request) {}
void WebInterface::apiGetQRCode(AsyncWebServerRequest* request) {}
void WebInterface::apiRestart(AsyncWebServerRequest* request) {}
void WebInterface::configWiFi(AsyncWebServerRequest* request, const JsonObject& config) {}
void WebInterface::configLightningWallet(AsyncWebServerRequest* request, const JsonObject& config) {}
void WebInterface::configColdStorage(AsyncWebServerRequest* request, const JsonObject& config) {}
void WebInterface::configDisplay(AsyncWebServerRequest* request, const JsonObject& config) {}
void WebInterface::configPower(AsyncWebServerRequest* request, const JsonObject& config) {}
void WebInterface::configSystem(AsyncWebServerRequest* request, const JsonObject& config) {} 