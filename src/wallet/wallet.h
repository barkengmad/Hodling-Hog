#ifndef WALLET_H
#define WALLET_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <mbedtls/md.h>  // For HMAC-SHA256 authentication

// Wallet of Satoshi API configuration
#define WOS_API_BASE_URL    "https://www.walletofsatoshi.com"
#define WOS_API_TIMEOUT     10000  // API timeout in milliseconds
#define WOS_RETRY_ATTEMPTS  3      // Number of retry attempts
#define WOS_RETRY_DELAY     2000   // Delay between retries in milliseconds

// Lightning transaction limits (in satoshis)
#define MIN_LIGHTNING_AMOUNT  1     // Minimum 1 satoshi
#define MAX_LIGHTNING_AMOUNT  1000000  // Maximum 1M sats (adjust as needed)

// Lightning wallet status
enum class WalletStatus {
    UNINITIALIZED,
    CONNECTING,
    CONNECTED,
    AUTHENTICATED,
    ERROR_NETWORK,
    ERROR_AUTH,
    ERROR_API,
    OFFLINE
};

// Lightning transaction types
enum class TransactionType {
    RECEIVE,
    SEND,
    INTERNAL_TRANSFER
};

// Lightning invoice data
struct LightningInvoice {
    String paymentRequest;        // BOLT11 invoice
    String paymentHash;           // Payment hash
    uint64_t amount;              // Amount in satoshis
    String description;           // Invoice description
    unsigned long expiry;         // Expiry timestamp
    bool paid;                    // Payment status
};

// Lightning balance data
struct LightningBalance {
    uint64_t confirmed;           // Confirmed balance in satoshis
    uint64_t pending;             // Pending balance in satoshis
    uint64_t total;               // Total balance in satoshis
    bool valid;                   // Whether data is current
    unsigned long lastUpdate;     // Last update timestamp
};

// Lightning transaction record
struct LightningTransaction {
    String txid;                  // Transaction ID
    TransactionType type;         // Transaction type
    uint64_t amount;              // Amount in satoshis
    String description;           // Transaction description
    unsigned long timestamp;      // Transaction timestamp
    bool confirmed;               // Confirmation status
};

class LightningWallet {
public:
    LightningWallet();
    void init();
    void setApiToken(const String& token);
    void setApiSecret(const String& secret);
    void setBaseUrl(const String& url);
    
    // WoS wallet management
    bool createWalletIfNeeded();
    bool isWalletCreated() const;
    
    // Wallet operations
    bool connect();
    void disconnect();
    bool authenticate();
    WalletStatus getStatus() const { return status; }
    
    // Balance operations
    bool updateBalance();
    LightningBalance getBalance() const;
    bool isBalanceValid() const;
    
    // Invoice operations
    LightningInvoice createInvoice(uint64_t amount, const String& description = "");
    bool checkInvoiceStatus(const String& paymentHash);
    String getReceiveAddress();
    
    // Payment operations
    bool sendPayment(const String& paymentRequest);
    bool sendToAddress(const String& address, uint64_t amount);
    
    // Transaction history
    bool updateTransactionHistory();
    std::vector<LightningTransaction> getRecentTransactions(int count = 10);
    
    // Transfer to cold storage
    bool transferToColdStorage(const String& address, uint64_t amount);
    String createWithdrawalRequest(const String& address, uint64_t amount);
    
    // Utility functions
    bool isConnected() const;
    String getLastError() const { return lastError; }
    unsigned long getLastUpdateTime() const { return balance.lastUpdate; }
    
    // Configuration
    void setTimeout(unsigned long timeout);
    void setRetryAttempts(int attempts);
    void enableAutoRetry(bool enable);
    
private:
    String apiToken;
    String apiSecret;  // WoS API secret for HMAC signing
    String baseUrl;
    WalletStatus status;
    LightningBalance balance;
    std::vector<LightningTransaction> transactions;
    
    // HTTP client for API calls
    HTTPClient httpClient;
    WiFiClientSecure secureClient;
    
    // Configuration
    unsigned long apiTimeout;
    int retryAttempts;
    int retryDelay;
    bool autoRetryEnabled;
    
    // Error handling
    String lastError;
    int lastHttpCode;
    unsigned long lastApiCall;
    
    // Private API methods
    bool makeApiCall(const String& endpoint, const String& method, const String& payload, String& response);
    bool makeGetRequest(const String& endpoint, String& response);
    bool makePostRequest(const String& endpoint, const String& payload, String& response);
    
    // JSON parsing helpers
    bool parseBalanceResponse(const String& response);
    bool parseInvoiceResponse(const String& response, LightningInvoice& invoice);
    bool parseTransactionResponse(const String& response);
    bool parsePaymentResponse(const String& response);
    
    // Validation helpers
    bool validateAmount(uint64_t amount);
    bool validatePaymentRequest(const String& paymentRequest);
    bool validateAddress(const String& address);
    
    // Authentication and headers
    void setAuthHeaders();
    String buildAuthHeader();
    
    // Error handling and logging
    void handleApiError(int httpCode, const String& response);
    void logApiCall(const String& endpoint, const String& method, int responseCode);
    void setError(const String& error);
    void clearError();
    
    // Retry logic
    bool retryApiCall(const String& endpoint, const String& method, const String& payload, String& response);
    void delayRetry(int attempt);
    
    // Utility methods
    String formatSatoshis(uint64_t satoshis);
    uint64_t parseSatoshis(const String& amount);
    String getCurrentTimestamp();
    bool isValidJson(const String& json);
    
    // WoS-specific methods
    bool createWoSWallet();
    String generateNonce();
    String calculateHMAC(const String& message, const String& key);
    bool makeWoSGetRequest(const String& endpoint, String& response);
    bool makeWoSPostRequest(const String& endpoint, const String& payload, String& response);
    bool parseWalletCreationResponse(const String& response, String& token, String& secret, String& address);
};

// Global instance
extern LightningWallet lightningWallet;

#endif // WALLET_H 