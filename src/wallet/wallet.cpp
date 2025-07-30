#include "wallet.h"
#include "../settings/settings.h"

// Global instance
LightningWallet lightningWallet;

LightningWallet::LightningWallet() {
    status = WalletStatus::UNINITIALIZED;
    apiTimeout = WOS_API_TIMEOUT;
    retryAttempts = WOS_RETRY_ATTEMPTS;
    retryDelay = WOS_RETRY_DELAY;
    autoRetryEnabled = true;
    lastHttpCode = 0;
    lastApiCall = 0;
    
    // Initialize balance
    balance.confirmed = 0;
    balance.pending = 0;
    balance.total = 0;
    balance.valid = false;
    balance.lastUpdate = 0;
}

void LightningWallet::init() {
    Serial.println("LightningWallet: Initializing");
    status = WalletStatus::UNINITIALIZED;
}

void LightningWallet::setApiToken(const String& token) {
    apiToken = token;
    Serial.println("LightningWallet: API token set");
}

void LightningWallet::setApiSecret(const String& secret) {
    apiSecret = secret;
    Serial.println("LightningWallet: API secret set");
}

void LightningWallet::setBaseUrl(const String& url) {
    baseUrl = url;
    Serial.printf("LightningWallet: Base URL set to %s\n", url.c_str());
}

bool LightningWallet::connect() {
    Serial.println("LightningWallet: Connecting...");
    status = WalletStatus::CONNECTED;
    return true;
}

void LightningWallet::disconnect() {
    Serial.println("LightningWallet: Disconnected");
    status = WalletStatus::UNINITIALIZED;
}

bool LightningWallet::authenticate() {
    Serial.println("LightningWallet: Authenticating...");
    status = WalletStatus::AUTHENTICATED;
    return true;
}

bool LightningWallet::updateBalance() {
    Serial.println("LightningWallet: Updating balance...");
    
    if (apiToken.isEmpty()) {
        Serial.println("LightningWallet: No API token configured - wallet not set up");
        balance.valid = false;
        return false;
    }
    
    // TODO: Implement real WoS API balance checking
    // For now, return a placeholder since WoS API endpoints need verification
    Serial.println("LightningWallet: Balance checking not yet implemented - showing placeholder");
    balance.confirmed = 0;
    balance.pending = 0;
    balance.total = 0;
    balance.valid = true;
    balance.lastUpdate = millis();
    
    return true;
}

LightningBalance LightningWallet::getBalance() const {
    return balance;
}

bool LightningWallet::isBalanceValid() const {
    return balance.valid;
}

LightningInvoice LightningWallet::createInvoice(uint64_t amount, const String& description) {
    LightningInvoice invoice;
    invoice.amount = amount;
    invoice.description = description;
    invoice.paymentRequest = "lnbc" + String(amount) + "..."; // Stub
    invoice.paymentHash = "abcd1234"; // Stub
    invoice.expiry = millis() + 3600000; // 1 hour
    invoice.paid = false;
    
    Serial.printf("LightningWallet: Invoice created for %llu sats\n", amount);
    return invoice;
}

bool LightningWallet::checkInvoiceStatus(const String& paymentHash) {
    Serial.printf("LightningWallet: Checking invoice %s\n", paymentHash.c_str());
    return false; // Stub
}

String LightningWallet::getReceiveAddress() {
    // Return the Lightning address from settings if available
    String address = settings.getConfig().lightning.receiveAddress;
    return address.isEmpty() ? "Not configured" : address;
}

bool LightningWallet::sendPayment(const String& paymentRequest) {
    Serial.printf("LightningWallet: Sending payment %s\n", paymentRequest.c_str());
    return true; // Stub
}

bool LightningWallet::sendToAddress(const String& address, uint64_t amount) {
    Serial.printf("LightningWallet: Sending %llu sats to %s\n", amount, address.c_str());
    return true; // Stub
}

bool LightningWallet::updateTransactionHistory() {
    Serial.println("LightningWallet: Updating transaction history");
    return true; // Stub
}

std::vector<LightningTransaction> LightningWallet::getRecentTransactions(int count) {
    return transactions; // Return empty vector for now
}

bool LightningWallet::transferToColdStorage(const String& address, uint64_t amount) {
    Serial.printf("LightningWallet: Transferring %llu sats to cold storage %s\n", amount, address.c_str());
    return true; // Stub
}

String LightningWallet::createWithdrawalRequest(const String& address, uint64_t amount) {
    return "withdrawal_request_id_123"; // Stub
}

bool LightningWallet::isConnected() const {
    return status == WalletStatus::CONNECTED || status == WalletStatus::AUTHENTICATED;
}

void LightningWallet::setTimeout(unsigned long timeout) {
    apiTimeout = timeout;
}

void LightningWallet::setRetryAttempts(int attempts) {
    retryAttempts = attempts;
}

void LightningWallet::enableAutoRetry(bool enable) {
    autoRetryEnabled = enable;
}

// Private stub methods
bool LightningWallet::makeApiCall(const String& endpoint, const String& method, const String& payload, String& response) {
    return true; // Stub
}

bool LightningWallet::makeGetRequest(const String& endpoint, String& response) {
    return true; // Stub  
}

bool LightningWallet::makePostRequest(const String& endpoint, const String& payload, String& response) {
    return true; // Stub
}

// parseBalanceResponse implementation moved to WoS section below

bool LightningWallet::parseInvoiceResponse(const String& response, LightningInvoice& invoice) {
    return true; // Stub
}

bool LightningWallet::parseTransactionResponse(const String& response) {
    return true; // Stub
}

bool LightningWallet::parsePaymentResponse(const String& response) {
    return true; // Stub
}

bool LightningWallet::validateAmount(uint64_t amount) {
    return amount >= MIN_LIGHTNING_AMOUNT && amount <= MAX_LIGHTNING_AMOUNT;
}

bool LightningWallet::validatePaymentRequest(const String& paymentRequest) {
    return paymentRequest.startsWith("lnbc");
}

bool LightningWallet::validateAddress(const String& address) {
    return address.length() > 0;
}

void LightningWallet::setAuthHeaders() {
    // Stub
}

String LightningWallet::buildAuthHeader() {
    return "Bearer " + apiToken;
}

void LightningWallet::handleApiError(int httpCode, const String& response) {
    lastHttpCode = httpCode;
    setError("API Error: " + String(httpCode));
}

void LightningWallet::logApiCall(const String& endpoint, const String& method, int responseCode) {
    Serial.printf("LightningWallet: %s %s -> %d\n", method.c_str(), endpoint.c_str(), responseCode);
}

void LightningWallet::setError(const String& error) {
    lastError = error;
    Serial.printf("LightningWallet: Error - %s\n", error.c_str());
}

void LightningWallet::clearError() {
    lastError = "";
}

bool LightningWallet::retryApiCall(const String& endpoint, const String& method, const String& payload, String& response) {
    return false; // Stub
}

void LightningWallet::delayRetry(int attempt) {
    delay(retryDelay * attempt);
}

String LightningWallet::formatSatoshis(uint64_t satoshis) {
    return String(satoshis) + " sats";
}

uint64_t LightningWallet::parseSatoshis(const String& amount) {
    return amount.toInt();
}

String LightningWallet::getCurrentTimestamp() {
    return String(millis());
}

bool LightningWallet::isValidJson(const String& json) {
    return json.startsWith("{") && json.endsWith("}");
}

// ============================================================================
// WoS (Wallet of Satoshi) API Implementation
// ============================================================================

bool LightningWallet::createWalletIfNeeded() {
    // Load existing credentials from settings if available
    if (!settings.getConfig().lightning.apiToken.isEmpty()) {
        apiToken = settings.getConfig().lightning.apiToken;
        apiSecret = settings.getConfig().lightning.apiSecret;
        Serial.println("LightningWallet: Using configured WoS credentials");
        return true;
    }
    
    // No credentials configured - user needs to add them manually
    Serial.println("LightningWallet: No WoS credentials configured. Please add API token in settings.");
    return false;
}

bool LightningWallet::isWalletCreated() const {
    return !apiToken.isEmpty() && !apiSecret.isEmpty();
}

bool LightningWallet::createWoSWallet() {
    // WoS does not support programmatic wallet creation
    // Users must manually get credentials from the WoS app
    Serial.println("LightningWallet: WoS wallet creation not supported - use manual credentials");
    Serial.println("LightningWallet: Please get API credentials from Wallet of Satoshi app and enter them in Settings");
    setError("Manual setup required - get WoS credentials from app");
    return false;
}

String LightningWallet::generateNonce() {
    // Generate a simple nonce using current time and random number
    return String(millis()) + String(random(1000000));
}

String LightningWallet::calculateHMAC(const String& message, const String& key) {
    // Calculate HMAC-SHA256 for WoS authentication
    uint8_t hmacResult[32];
    
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)key.c_str(), key.length());
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)message.c_str(), message.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
    
    // Convert to hex string
    String hmacHex = "";
    for (int i = 0; i < 32; i++) {
        if (hmacResult[i] < 16) hmacHex += "0";
        hmacHex += String(hmacResult[i], HEX);
    }
    
    return hmacHex;
}

bool LightningWallet::makeWoSGetRequest(const String& endpoint, String& response) {
    if (apiToken.isEmpty()) {
        setError("No API token configured");
        return false;
    }
    
    HTTPClient http;
    http.setTimeout(WOS_API_TIMEOUT);
    
    String url = String(WOS_API_BASE_URL) + endpoint;
    if (!http.begin(url)) {
        setError("Failed to initialize HTTP client");
        return false;
    }
    
    // Add WoS authentication headers
    http.addHeader("Authorization", "Bearer " + apiToken);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "HodlingHog/1.0");
    
    int httpCode = http.GET();
    lastHttpCode = httpCode;
    
    if (httpCode == 200) {
        response = http.getString();
        Serial.printf("LightningWallet: GET %s - Success (%d bytes)\n", endpoint.c_str(), response.length());
        http.end();
        clearError();
        return true;
    } else {
        Serial.printf("LightningWallet: GET %s - Failed (HTTP %d)\n", endpoint.c_str(), httpCode);
        setError("GET request failed: HTTP " + String(httpCode));
        http.end();
        return false;
    }
}

bool LightningWallet::makeWoSPostRequest(const String& endpoint, const String& payload, String& response) {
    if (apiToken.isEmpty() || apiSecret.isEmpty()) {
        setError("No API credentials configured");
        return false;
    }
    
    HTTPClient http;
    http.setTimeout(WOS_API_TIMEOUT);
    
    String url = String(WOS_API_BASE_URL) + endpoint;
    if (!http.begin(url)) {
        setError("Failed to initialize HTTP client");
        return false;
    }
    
    // Generate nonce and HMAC signature for WoS authentication
    String nonce = generateNonce();
    String message = endpoint + nonce + payload;
    String signature = calculateHMAC(message, apiSecret);
    
    // Add WoS authentication headers
    http.addHeader("Authorization", "Bearer " + apiToken);
    http.addHeader("X-Nonce", nonce);
    http.addHeader("X-Signature", signature);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "HodlingHog/1.0");
    
    int httpCode = http.POST(payload);
    lastHttpCode = httpCode;
    
    if (httpCode == 200 || httpCode == 201) {
        response = http.getString();
        Serial.printf("LightningWallet: POST %s - Success (%d bytes)\n", endpoint.c_str(), response.length());
        http.end();
        clearError();
        return true;
    } else {
        Serial.printf("LightningWallet: POST %s - Failed (HTTP %d)\n", endpoint.c_str(), httpCode);
        String errorResponse = http.getString();
        Serial.printf("LightningWallet: Error response: %s\n", errorResponse.c_str());
        setError("POST request failed: HTTP " + String(httpCode));
        http.end();
        return false;
    }
}

bool LightningWallet::parseWalletCreationResponse(const String& response, String& token, String& secret, String& address) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.printf("LightningWallet: JSON parsing failed: %s\n", error.c_str());
        setError("Invalid JSON response");
        return false;
    }
    
    // Parse WoS wallet creation response
    if (doc["success"].as<bool>() && doc.containsKey("data")) {
        JsonObject data = doc["data"];
        
        if (data.containsKey("api_token") && data.containsKey("api_secret")) {
            token = data["api_token"].as<String>();
            secret = data["api_secret"].as<String>();
            
            // Lightning address format: usually generated from token
            if (data.containsKey("lightning_address")) {
                address = data["lightning_address"].as<String>();
            } else {
                // Generate lightning address format
                address = token.substring(0, 8) + "@getalby.com";
            }
            
            Serial.println("LightningWallet: Wallet creation response parsed successfully");
            return true;
        }
    }
    
    Serial.println("LightningWallet: Invalid wallet creation response format");
    setError("Invalid wallet creation response");
    return false;
}

bool LightningWallet::parseBalanceResponse(const String& response) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.printf("LightningWallet: JSON parsing failed: %s\n", error.c_str());
        setError("Invalid JSON response");
        balance.valid = false;
        return false;
    }
    
    // Parse WoS balance response
    if (doc["success"].as<bool>() && doc.containsKey("data")) {
        JsonObject data = doc["data"];
        
        if (data.containsKey("balance")) {
            // WoS returns balance in satoshis
            balance.confirmed = data["balance"].as<uint64_t>();
            balance.pending = data.containsKey("pending") ? data["pending"].as<uint64_t>() : 0;
            balance.total = balance.confirmed + balance.pending;
            balance.valid = true;
            balance.lastUpdate = millis();
            
            Serial.printf("LightningWallet: Balance parsed successfully - %llu sats\n", balance.total);
            clearError();
            return true;
        }
    }
    
    Serial.println("LightningWallet: Invalid balance response format");
    setError("Invalid balance response");
    balance.valid = false;
    return false;
} 