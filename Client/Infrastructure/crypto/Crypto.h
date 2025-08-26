#pragma "once"

#include "string"
#include "vector"
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/err.h>


class Crypto {
public:
    Crypto() { OpenSSL_add_all_algorithms(); ERR_load_crypto_strings(); }
    ~Crypto() { /*cleanup handled by OS*/ }

    // Генерация RSA 2048 и выдача PEM строк
    bool generate_rsa_pem(std::string &priv_pem, std::string &pub_pem) {
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        if (!pctx) return false;
        if (EVP_PKEY_keygen_init(pctx) <= 0) return false;
        if (EVP_PKEY_CTX_set_rsa_keygen_bits(pctx, 2048) <= 0) return false;
        EVP_PKEY* pkey = NULL;
        if (EVP_PKEY_keygen(pctx, &pkey) <= 0) { EVP_PKEY_CTX_free(pctx); return false; }
        EVP_PKEY_CTX_free(pctx);

        BIO *bpriv = BIO_new(BIO_s_mem());
        BIO *bpub = BIO_new(BIO_s_mem());
        PEM_write_bio_PrivateKey(bpriv, pkey, NULL, NULL, 0, 0, NULL);
        PEM_write_bio_PUBKEY(bpub, pkey);

        char *p; long n;
        n = BIO_get_mem_data(bpriv, &p); priv_pem.assign(p, n);
        n = BIO_get_mem_data(bpub, &p); pub_pem.assign(p, n);
        BIO_free(bpriv); BIO_free(bpub);
        EVP_PKEY_free(pkey);
        return true;
    }

    // Загрузка EVP_PKEY из PEM
    EVP_PKEY* load_pubkey_from_pem(const std::string &pem) {
        BIO* bio = BIO_new_mem_buf(pem.data(), (int)pem.size());
        EVP_PKEY* p = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
        BIO_free(bio);
        return p;
    }
    EVP_PKEY* load_privkey_from_pem(const std::string &pem) {
        BIO* bio = BIO_new_mem_buf(pem.data(), (int)pem.size());
        EVP_PKEY* p = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        BIO_free(bio);
        return p;
    }

    // RSA encrypt (OAEP) using recipient public key
    bool rsa_encrypt(EVP_PKEY* pub, const std::vector<unsigned char> &in, std::vector<unsigned char> &out) {
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pub, NULL);
        if (!ctx) return false;
        if (EVP_PKEY_encrypt_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        size_t outlen;
        if (EVP_PKEY_encrypt(ctx, NULL, &outlen, in.data(), in.size()) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        out.resize(outlen);
        if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, in.data(), in.size()) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        out.resize(outlen);
        EVP_PKEY_CTX_free(ctx);
        return true;
    }
    // RSA decrypt (OAEP)
    bool rsa_decrypt(EVP_PKEY* priv, const std::vector<unsigned char> &in, std::vector<unsigned char> &out) {
        EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(priv, NULL);
        if (!ctx) return false;
        if (EVP_PKEY_decrypt_init(ctx) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        size_t outlen;
        if (EVP_PKEY_decrypt(ctx, NULL, &outlen, in.data(), in.size()) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        out.resize(outlen);
        if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, in.data(), in.size()) <= 0) { EVP_PKEY_CTX_free(ctx); return false; }
        out.resize(outlen);
        EVP_PKEY_CTX_free(ctx);
        return true;
    }

    // AES-256-GCM encrypt: key 32 bytes, iv 12 bytes
    bool aes_gcm_encrypt(const std::vector<unsigned char>& key, const std::vector<unsigned char>& plaintext,
                         std::vector<unsigned char>& iv, std::vector<unsigned char>& ciphertext, std::vector<unsigned char>& tag) {
        if (key.size() != 32) return false;
        iv.resize(12);
        if (!RAND_bytes(iv.data(), (int)iv.size())) return false;
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;
        int len;
        if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) { EVP_CIPHER_CTX_free(ctx); return false; }
        if (!EVP_EncryptInit_ex(ctx, NULL, NULL, key.data(), iv.data())) { EVP_CIPHER_CTX_free(ctx); return false; }
        ciphertext.resize(plaintext.size());
        if (!EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), (int)plaintext.size())) { EVP_CIPHER_CTX_free(ctx); return false; }
        int ciphertext_len = len;
        if (!EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) { EVP_CIPHER_CTX_free(ctx); return false; }
        ciphertext_len += len;
        ciphertext.resize(ciphertext_len);
        tag.resize(16);
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data())) { EVP_CIPHER_CTX_free(ctx); return false; }
        EVP_CIPHER_CTX_free(ctx);
        return true;
    }
    // AES-256-GCM decrypt
    bool aes_gcm_decrypt(const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv,
                         const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& tag,
                         std::vector<unsigned char>& plaintext) {
        if (key.size() != 32 || iv.size() != 12 || tag.size() != 16) return false;
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;
        int len;
        if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) { EVP_CIPHER_CTX_free(ctx); return false; }
        if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key.data(), iv.data())) { EVP_CIPHER_CTX_free(ctx); return false; }
        plaintext.resize(ciphertext.size());
        if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), (int)ciphertext.size())) { EVP_CIPHER_CTX_free(ctx); return false; }
        int plaintext_len = len;
        if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag.data())) { EVP_CIPHER_CTX_free(ctx); return false; }
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) <= 0) { EVP_CIPHER_CTX_free(ctx); return false; }
        plaintext_len += len;
        plaintext.resize(plaintext_len);
        EVP_CIPHER_CTX_free(ctx);
        return true;
    }

    // Генерация случайного AES ключа 32 байта
    static std::vector<unsigned char> gen_aes_key() {
        std::vector<unsigned char> k(32);
        RAND_bytes(k.data(), (int)k.size());
        return k;
    }
};