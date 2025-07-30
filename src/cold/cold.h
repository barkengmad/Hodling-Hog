#ifndef COLD_H
#define COLD_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

// Cold storage API configuration
#define COLD_API_TIMEOUT     15000  // API timeout in milliseconds
#define COLD_RETRY_ATTEMPTS  3      // Number of retry attempts
#define COLD_RETRY_DELAY     3000   // Delay between retries in milliseconds

// Bitcoin transaction limits
#define MIN_BITCOIN_AMOUNT   546    // Dust limit in satoshis
#define MAX_BITCOIN_AMOUNT   2100000000000000LL  // 21M BTC in satoshis

// Cold storage status
enum class ColdStorageStatus {
    UNINITIALIZED,
    CONNECTING,
    CONNECTED,
    SYNCHRONIZED,
    ERROR_NETWORK,
    ERROR_API,
    ERROR_ADDRESS,
    OFFLINE
};

// Transaction status
enum class TxStatus {
    UNCONFIRMED,
    CONFIRMED,
    FAILED,
    PENDING
};

// UTXO (Unspent Transaction Output) data
struct UTXO {
    String txid;                  // Transaction ID
    uint32_t vout;                // Output index
    uint64_t value;               // Value in satoshis
    String scriptPubKey;          // Script public key
    uint32_t confirmations;       // Number of confirmations
    bool spendable;               // Whether UTXO is spendable
};

// Cold storage balance data
struct ColdBalance {
    uint64_t confirmed;           // Confirmed balance in satoshis
    uint64_t unconfirmed;         // Unconfirmed balance in satoshis
    uint64_t total;               // Total balance in satoshis
    uint32_t txCount;             // Total transaction count
    bool valid;                   // Whether data is current
    unsigned long lastUpdate;     // Last update timestamp
};

// Bitcoin transaction data
struct BitcoinTransaction {
    String txid;                  // Transaction ID
    uint64_t amount;              // Amount in satoshis
    String address;               // Address involved
    TxStatus status;              // Transaction status
    uint32_t confirmations;       // Number of confirmations
    unsigned long timestamp;      // Transaction timestamp
    uint64_t fee;                 // Transaction fee in satoshis
    bool isIncoming;              // Whether transaction is incoming
};

// Transaction building data
struct TransactionBuilder {
    String toAddress;             // Destination address
    uint64_t amount;              // Amount to send in satoshis
    uint64_t feeRate;             // Fee rate in sat/vB
    std::vector<UTXO> inputs;     // Input UTXOs
    String rawTx;                 // Raw transaction hex
    String txid;                  // Transaction ID after broadcast
    bool isSigned;                // Whether transaction is signed
};

class ColdStorage {
public:
    ColdStorage();
    void init();
    void setAddress(const String& address);
    void setPrivateKey(const String& privateKey);  // Optional for signing
    void setApiEndpoint(const String& endpoint);
    
    // Address and key management
    bool isValidAddress(const String& address);
    bool hasPrivateKey() const { return !privateKey.isEmpty(); }
    String getWatchAddress() const { return watchAddress; }
    
    // Connection and synchronization
    bool connect();
    void disconnect();
    ColdStorageStatus getStatus() const { return status; }
    
    // Balance operations
    bool updateBalance();
    ColdBalance getBalance() const;
    bool isBalanceValid() const;
    
    // UTXO management
    bool updateUTXOs();
    std::vector<UTXO> getUTXOs() const;
    uint64_t getSpendableBalance();
    
    // Transaction history
    bool updateTransactionHistory();
    std::vector<BitcoinTransaction> getTransactions(int count = 10);
    BitcoinTransaction getTransactionDetails(const String& txid);
    
    // Transaction building and signing
    TransactionBuilder createTransaction(const String& toAddress, uint64_t amount, uint64_t feeRate = 0);
    bool signTransaction(TransactionBuilder& txBuilder);
    String exportUnsignedTransaction(const TransactionBuilder& txBuilder);
    bool importSignedTransaction(const String& signedTxHex);
    
    // Broadcasting
    bool broadcastTransaction(const String& rawTx);
    bool sendTransaction(const String& toAddress, uint64_t amount, uint64_t feeRate = 0);
    
    // Fee estimation
    uint64_t estimateFee(uint64_t amount, uint64_t feeRate = 0);
    uint64_t getCurrentFeeRate();
    uint64_t getMinimumFeeRate();
    
    // QR code generation for signing
    String generateSigningQR(const TransactionBuilder& txBuilder);
    String generateAddressQR();
    
    // Utility functions
    bool isConnected() const;
    String getLastError() const { return lastError; }
    unsigned long getLastUpdateTime() const { return balance.lastUpdate; }
    
    // Configuration
    void setTimeout(unsigned long timeout);
    void setRetryAttempts(int attempts);
    void enableTestnet(bool enable);
    
private:
    String watchAddress;
    String privateKey;
    String apiEndpoint;
    ColdStorageStatus status;
    ColdBalance balance;
    std::vector<UTXO> utxos;
    std::vector<BitcoinTransaction> transactions;
    
    // HTTP client for API calls
    HTTPClient httpClient;
    WiFiClientSecure secureClient;
    
    // Configuration
    unsigned long apiTimeout;
    int retryAttempts;
    int retryDelay;
    bool testnetEnabled;
    
    // Error handling
    String lastError;
    int lastHttpCode;
    unsigned long lastApiCall;
    
    // Private API methods
    bool makeApiCall(const String& endpoint, const String& method, const String& payload, String& response);
    bool makeGetRequest(const String& endpoint, String& response);
    bool makePostRequest(const String& endpoint, const String& payload, String& response);
    
    // Specific API calls
    bool fetchAddressBalance(const String& address);
    bool fetchAddressUTXOs(const String& address);
    bool fetchAddressTransactions(const String& address);
    bool fetchTransactionDetails(const String& txid);
    bool fetchFeeEstimates();
    
    // JSON parsing helpers
    bool parseBalanceResponse(const String& response);
    bool parseUTXOResponse(const String& response);
    bool parseTransactionResponse(const String& response);
    bool parseFeeResponse(const String& response);
    
    // Transaction building helpers
    std::vector<UTXO> selectUTXOs(uint64_t amount, uint64_t feeRate);
    uint32_t calculateTxSize(int inputCount, int outputCount);
    uint64_t calculateRequiredFee(uint32_t txSize, uint64_t feeRate);
    String buildRawTransaction(const std::vector<UTXO>& inputs, const String& toAddress, uint64_t amount, uint64_t change);
    
    // Validation helpers
    bool validateAmount(uint64_t amount);
    bool validateFeeRate(uint64_t feeRate);
    bool validateTransaction(const TransactionBuilder& txBuilder);
    
    // Cryptographic functions (if private key is available)
    String signTransactionHash(const String& txHash);
    bool verifySignature(const String& signature, const String& hash);
    String derivePublicKey();
    String deriveAddress();
    
    // Error handling and logging
    void handleApiError(int httpCode, const String& response);
    void logApiCall(const String& endpoint, const String& method, int responseCode);
    void setError(const String& error);
    void clearError();
    
    // Utility methods
    String formatSatoshis(uint64_t satoshis);
    uint64_t parseSatoshis(const String& amount);
    String getCurrentTimestamp();
    bool isValidJson(const String& json);
    String bytesToHex(const uint8_t* bytes, size_t length);
    void hexToBytes(const String& hex, uint8_t* bytes);
    
    // Address validation
    bool isValidBitcoinAddress(const String& address);
    bool isValidPrivateKey(const String& key);
    String getAddressType(const String& address);
};

// Global instance
extern ColdStorage coldStorage;

#endif // COLD_H 