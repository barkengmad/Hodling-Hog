#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <map>

// Web server configuration
#define WEB_SERVER_PORT     80
#define CONFIG_AP_TIMEOUT   300000  // 5 minutes in AP mode
#define MAX_CLIENTS         4       // Maximum concurrent clients
#define SESSION_TIMEOUT     1800000 // 30 minutes session timeout

// Web interface status
enum class WebStatus {
    STOPPED,
    STARTING,
    RUNNING,
    AP_MODE,
    ERROR_INIT,
    ERROR_NETWORK
};

// Authentication levels
enum class AuthLevel {
    NONE,
    BASIC,
    ADMIN
};

// Configuration sections
enum class ConfigSection {
    WIFI,
    LIGHTNING_WALLET,
    COLD_STORAGE,
    DISPLAY_CONFIG,
    POWER,
    SYSTEM
};

// Web request context
struct WebContext {
    String clientIP;
    String userAgent;
    AuthLevel authLevel;
    unsigned long sessionStart;
    String sessionToken;
};

class WebInterface {
public:
    WebInterface();
    void init();
    void start();
    void stop();
    void loop();
    
    // Server management
    bool isRunning() const { return status == WebStatus::RUNNING; }
    WebStatus getStatus() const { return status; }
    void enableCaptivePortal(bool enable);
    
    // Access Point mode
    void startAPMode();
    void stopAPMode();
    bool isAPMode() const { return apModeActive; }
    String getAPIP() const;
    
    // Configuration interface
    void handleConfigRequest(AsyncWebServerRequest* request);
    void handleApiRequest(AsyncWebServerRequest* request);
    void handleFileUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final);
    
    // Wallet management
    void handleWalletConfig(AsyncWebServerRequest* request);
    void handleLightningTransfer(AsyncWebServerRequest* request);
    void handleColdStorageConfig(AsyncWebServerRequest* request);
    void handleSystemConfig(AsyncWebServerRequest* request);
    void handleTransactionSigning(AsyncWebServerRequest* request);
    
    // System management
    void handleSystemInfo(AsyncWebServerRequest* request);
    void handleSystemRestart(AsyncWebServerRequest* request);
    void handleFactoryReset(AsyncWebServerRequest* request);
    void handleFirmwareUpdate(AsyncWebServerRequest* request);
    
    // Authentication and security
    bool authenticateRequest(AsyncWebServerRequest* request, AuthLevel requiredLevel = AuthLevel::BASIC);
    String generateSessionToken();
    bool validateSessionToken(const String& token);
    void invalidateSession(const String& token);
    void clearAllSessions();
    
    // Utility methods
    String getDeviceInfo();
    String getSystemStatus();
    String getNetworkInfo();
    
private:
    AsyncWebServer server;
    WebStatus status;
    bool captivePortalEnabled;
    bool apModeActive;
    
    // AP mode configuration
    String apSSID;
    String apPassword;
    IPAddress apIP;
    IPAddress apGateway;
    IPAddress apSubnet;
    
    // Session management
    std::map<String, WebContext> activeSessions;
    unsigned long lastSessionCleanup;
    
    // Configuration
    unsigned long apTimeout;
    uint8_t maxClients;
    bool authRequired;
    String adminPassword;
    
    // Temporary storage for seed phrase generation flow
    String pendingSeedPhrase;
    
    // Request handlers
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest* request);
    void handleConfig(AsyncWebServerRequest* request);
    void handleAPI(AsyncWebServerRequest* request);
    void handleNotFound(AsyncWebServerRequest* request);
    void handleCaptivePortal(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);
    void handleLogout(AsyncWebServerRequest* request);
    void handleSetup(AsyncWebServerRequest* request);
    void handleGenerateSeed(AsyncWebServerRequest* request);
    void handleConfirmSeed(AsyncWebServerRequest* request);
    
    // API endpoints
    void apiGetStatus(AsyncWebServerRequest* request);
    void apiGetBalances(AsyncWebServerRequest* request);
    void apiUpdateBalances(AsyncWebServerRequest* request);
    void apiGetConfig(AsyncWebServerRequest* request);
    void apiSetConfig(AsyncWebServerRequest* request);
    void apiGetTransactions(AsyncWebServerRequest* request);
    void apiCreateInvoice(AsyncWebServerRequest* request);
    void apiSendPayment(AsyncWebServerRequest* request);
    void apiTransferFunds(AsyncWebServerRequest* request);
    void apiSignTransaction(AsyncWebServerRequest* request);
    void apiGetQRCode(AsyncWebServerRequest* request);
    void apiRestart(AsyncWebServerRequest* request);
    
    // Configuration handlers
    void configWiFi(AsyncWebServerRequest* request, const JsonObject& config);
    void configLightningWallet(AsyncWebServerRequest* request, const JsonObject& config);
    void configColdStorage(AsyncWebServerRequest* request, const JsonObject& config);
    void configDisplay(AsyncWebServerRequest* request, const JsonObject& config);
    void configPower(AsyncWebServerRequest* request, const JsonObject& config);
    void configSystem(AsyncWebServerRequest* request, const JsonObject& config);
    
    // Form submission handlers
    void handleWiFiConfig(AsyncWebServerRequest* request);
    void handleLightningConfig(AsyncWebServerRequest* request);
    
    // HTML page generators
    String generateLandingPage();
    String generateMainPage();
    String generateConfigPage();
    // String generateWalletPage();      // Commented out - passive mode
    // String generateTransferPage();    // Commented out - passive mode
    String generateSystemPage();
    String generateCaptivePortalPage();
    String generateLoginPage();
    String generateSetupPage();
    String generateSeedDisplayPage(const String& seedPhrase);
    String generateSeedConfirmPage();
    
    // CSS and JavaScript
    String getCSS();
    String getJavaScript();
    String getBootstrapCSS();
    String getJQuery();
    
    // JSON response helpers
    void sendJsonResponse(AsyncWebServerRequest* request, const JsonDocument& doc, int httpCode = 200);
    void sendErrorResponse(AsyncWebServerRequest* request, const String& error, int httpCode = 400);
    void sendSuccessResponse(AsyncWebServerRequest* request, const String& message = "OK");
    JsonDocument createStatusJson();
    JsonDocument createBalanceJson();
    JsonDocument createConfigJson();
    
    // Session management
    void cleanupSessions();
    WebContext* getSession(const String& token);
    WebContext* createSession(const String& clientIP, const String& userAgent);
    void updateSessionActivity(const String& token);
    
    // Security helpers
    bool checkRateLimit(const String& clientIP);
    void logSecurityEvent(const String& event, const String& clientIP);
    String hashPassword(const String& password);
    bool verifyPassword(const String& password, const String& hash);
    
    // File serving
    void serveStaticFile(AsyncWebServerRequest* request, const String& filename);
    String getContentType(const String& filename);
    bool fileExists(const String& path);
    
    // Error handling
    void handleWebError(const String& error);
    void logWebAccess(AsyncWebServerRequest* request, int responseCode);
    
    // Utility methods
    String urlDecode(const String& str);
    String urlEncode(const String& str);
    String getClientIP(AsyncWebServerRequest* request);
    String formatFileSize(size_t bytes);
    String formatUptime();
    bool isValidJSON(const String& json);
    
    // Network helpers
    void startMDNS();
    void stopMDNS();
    bool checkInternetConnection();
    String getWiFiStatusString();
    
    // Captive portal detection
    bool isCaptivePortalRequest(AsyncWebServerRequest* request);
    void redirectToCaptivePortal(AsyncWebServerRequest* request);
    void handleDNSRedirect();
};

// Global instance
extern WebInterface webInterface;

#endif // WEB_H 