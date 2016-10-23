#include <string.h> // memcmp
#include <anyrtc.h>
#include "dtls_transport.h"
#include "dtls_parameters.h"
#include "certificate.h"
#include "utils.h"

#define DEBUG_MODULE "dtls-transport"
#define DEBUG_LEVEL 7
#include <re_dbg.h>

/*
 * Embedded DH parameters in DER encoding (bits: 2048)
 */
uint8_t anyrtc_default_dh_parameters[] = {
    0x30, 0x82, 0x01, 0x08, 0x02, 0x82, 0x01, 0x01, 0x00, 0xaa, 0x4c, 0x1f,
    0x1e, 0xc9, 0xed, 0xfe, 0x5c, 0x50, 0x2d, 0xff, 0xf4, 0x95, 0xf4, 0x80,
    0x69, 0xcf, 0xc3, 0x84, 0x29, 0x87, 0xd5, 0x2c, 0x4f, 0xf6, 0x9e, 0x88,
    0xa2, 0x5b, 0x61, 0xd2, 0x7d, 0x78, 0x97, 0xce, 0x47, 0x39, 0x9d, 0xc0,
    0x95, 0x14, 0x98, 0x1f, 0xa9, 0xa3, 0x42, 0x93, 0x58, 0x49, 0x3d, 0xad,
    0xeb, 0x6c, 0x3d, 0x79, 0x2d, 0x27, 0x94, 0x67, 0x4c, 0xdc, 0x94, 0x31,
    0xbf, 0xc1, 0x00, 0x9d, 0x96, 0x4a, 0x91, 0xa7, 0x4f, 0xab, 0x48, 0x44,
    0xcc, 0x54, 0x1a, 0x4e, 0x2a, 0x8e, 0xa1, 0x81, 0x4b, 0xeb, 0xea, 0xc3,
    0xba, 0xd6, 0x03, 0xfb, 0xf2, 0x9a, 0x48, 0x1f, 0xc8, 0xba, 0x73, 0x89,
    0x86, 0x25, 0x2e, 0xba, 0x10, 0x80, 0x2a, 0xeb, 0xf9, 0xe2, 0x28, 0xf1,
    0xcf, 0x85, 0x0d, 0xeb, 0x2f, 0x61, 0x51, 0x11, 0xe1, 0xe7, 0x82, 0xe5,
    0xa7, 0x5d, 0x71, 0x0a, 0xef, 0x8a, 0xe1, 0x97, 0x48, 0x41, 0xac, 0xd7,
    0xc5, 0xf7, 0xce, 0xd5, 0xcd, 0x66, 0x1e, 0x6b, 0x0e, 0x82, 0x4e, 0x77,
    0x5d, 0x89, 0x3b, 0xe2, 0x94, 0x7a, 0x10, 0xee, 0x5b, 0x5d, 0x36, 0x07,
    0x29, 0x8b, 0x06, 0xb6, 0x49, 0x1e, 0x17, 0x17, 0x57, 0xc8, 0xc1, 0x80,
    0x24, 0x15, 0x22, 0x9c, 0xb8, 0x59, 0x55, 0x08, 0x41, 0x67, 0x07, 0xca,
    0xa8, 0x54, 0x1a, 0xd1, 0xb7, 0x91, 0x2f, 0x41, 0x78, 0xc0, 0xcd, 0x2f,
    0x07, 0x49, 0x4b, 0xb9, 0x05, 0xf4, 0xea, 0x72, 0x3a, 0xcf, 0x04, 0x69,
    0xcb, 0x5b, 0xe4, 0xcb, 0x4f, 0x72, 0x40, 0xe4, 0x56, 0x1f, 0xca, 0xee,
    0x33, 0x2b, 0x29, 0x1a, 0x80, 0xda, 0x01, 0x3f, 0x03, 0xa6, 0xbf, 0x32,
    0x02, 0x6c, 0xfb, 0xb1, 0xb5, 0x81, 0xda, 0x32, 0x6f, 0xa1, 0x4b, 0x9f,
    0x42, 0x2e, 0x17, 0xc9, 0x95, 0x30, 0xda, 0x16, 0xb7, 0x9a, 0x7c, 0xf4,
    0x83, 0x02, 0x01, 0x02
};
size_t const anyrtc_default_dh_parameters_length =
        sizeof(anyrtc_default_dh_parameters) / sizeof(*anyrtc_default_dh_parameters);

/*
 * List of default DTLS cipher suites.
 */
char const* anyrtc_default_dtls_cipher_suites[] = {
    "ECDHE-ECDSA-CHACHA20-POLY1305",
    "ECDHE-RSA-CHACHA20-POLY1305",
    "ECDHE-ECDSA-AES128-GCM-SHA256", // recommended
    "ECDHE-RSA-AES128-GCM-SHA256",
    "ECDHE-ECDSA-AES256-GCM-SHA384",
    "ECDHE-RSA-AES256-GCM-SHA384",
    "DHE-RSA-AES128-GCM-SHA256",
    "DHE-RSA-AES256-GCM-SHA384",
    "ECDHE-ECDSA-AES128-SHA256",
    "ECDHE-RSA-AES128-SHA256",
    "ECDHE-ECDSA-AES128-SHA", // required
    "ECDHE-RSA-AES256-SHA384",
    "ECDHE-RSA-AES128-SHA",
    "ECDHE-ECDSA-AES256-SHA384",
    "ECDHE-ECDSA-AES256-SHA",
    "ECDHE-RSA-AES256-SHA",
    "DHE-RSA-AES128-SHA256",
    "DHE-RSA-AES128-SHA",
    "DHE-RSA-AES256-SHA256",
    "DHE-RSA-AES256-SHA"
};
size_t const anyrtc_default_dtls_cipher_suites_length =
        sizeof(anyrtc_default_dtls_cipher_suites) / sizeof(*anyrtc_default_dtls_cipher_suites);

/*
 * Get the corresponding name for an ICE transport state.
 */
char const * const anyrtc_dtls_transport_state_to_name(
        enum anyrtc_dtls_transport_state const state
) {
    switch (state) {
        case ANYRTC_DTLS_TRANSPORT_STATE_NEW:
            return "new";
        case ANYRTC_DTLS_TRANSPORT_STATE_CONNECTING:
            return "connecting";
        case ANYRTC_DTLS_TRANSPORT_STATE_CONNECTED:
            return "connected";
        case ANYRTC_DTLS_TRANSPORT_STATE_CLOSED:
            return "closed";
        case ANYRTC_DTLS_TRANSPORT_STATE_FAILED:
            return "failed";
        default:
            return "???";
    }
}

/*
 * Change the state of the ICE transport.
 * Will call the corresponding handler.
 * Caller MUST ensure that the same state is not set twice.
 */
static enum anyrtc_code set_state(
        struct anyrtc_dtls_transport* const transport,
        enum anyrtc_dtls_transport_state const state
) {
    // Closed or failed: Remove connection
    if (state == ANYRTC_DTLS_TRANSPORT_STATE_CLOSED
            || state == ANYRTC_DTLS_TRANSPORT_STATE_FAILED) {
        // Remove connection
        transport->connection = mem_deref(transport->connection);

        // Remove self from ICE transport (if attached)
        transport->ice_transport->dtls_transport = NULL;
    }

    // Set state
    transport->state = state;

    // Call handler (if any)
    if (transport->state_change_handler) {
        transport->state_change_handler(state, transport->arg);
    }

    return ANYRTC_CODE_SUCCESS;
}

/*
 * Check if the state is 'closed' or 'failed'.
 */
static bool is_closed(
        struct anyrtc_dtls_transport* const transport // not checked
) {
    switch (transport->state) {
        case ANYRTC_DTLS_TRANSPORT_STATE_CLOSED:
        case ANYRTC_DTLS_TRANSPORT_STATE_FAILED:
            return true;
        default:
            return false;
    }
}

/*
 * DTLS connection closed handler.
 */
static void close_handler(
        int err,
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;
    enum anyrtc_code error;

    // Closed?
    if (!is_closed(transport)) {
        DEBUG_INFO("DTLS connection closed, reason: %m\n", err);

        // Set state
        // TODO: What about normal close?
        set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_FAILED);

        // Stop
        error = anyrtc_dtls_transport_stop(transport);
        if (error) {
            DEBUG_WARNING("DTLS connection closed, could not stop transport: %s\n",
                          anyrtc_code_to_str(error));
        }
    } else {
        DEBUG_PRINTF("DTLS connection closed (but state is already closed anyway), reason: %m\n",
                     err);
    }
}

/*
 * Handle incoming DTLS messages.
 */
static void dtls_receive_handler(
        struct mbuf *const buffer,
        void *const arg
) {
    (void) buffer;
    struct anyrtc_dtls_transport* const transport = arg;
    DEBUG_PRINTF("Received %zu bytes on DTLS connection\n", mbuf_get_left(buffer));

    // Check state
    if (is_closed(transport)) {
        DEBUG_PRINTF("Ignoring incoming DTLS message, transport is closed\n");
        return;
    }

    // TODO: Implement
    // TODO: Buffer if no handler, also buffer if state is not CONNECTED
    DEBUG_WARNING("TODO: Buffer DTLS message: %b\n", buffer->buf, mbuf_get_left(buffer));
}

/*
 * Either called by a DTLS connection established event or by the
 * `start` method of the DTLS transport.
 * The caller MUST make sure that remote parameters are available and
 * that the state is NOT 'closed' or 'failed'!
 */
static void verify_certificate(
        struct anyrtc_dtls_transport* const transport // not checked
) {
    enum anyrtc_code error;
    bool valid = false;
    struct le* le;
    enum tls_fingerprint algorithm;
    int err;
    uint8_t expected_fingerprint[ANYRTC_FINGERPRINT_MAX_SIZE];
    uint8_t actual_fingerprint[ANYRTC_FINGERPRINT_MAX_SIZE];

    // Verify the peer's certificate
    // TODO: Fix this. Testing the fingerprint alone is okay for now though.
//    error = anyrtc_translate_re_code(tls_peer_verify(transport->connection));
//    if (error) {
//        goto out;
//    }
//    DEBUG_PRINTF("Peer's certificate verified\n");

    // Check if any of the fingerprints provided matches
    // TODO: Is this correct?
    for (le = list_head(&transport->remote_parameters->fingerprints); le != NULL; le = le->next) {
        struct anyrtc_dtls_fingerprint* const fingerprint = le->data;
        size_t length;

        // Get algorithm
        error = anyrtc_translate_certificate_sign_algorithm(&algorithm, fingerprint->algorithm);
        if (error) {
            if (error == ANYRTC_CODE_UNSUPPORTED_ALGORITHM) {
                continue;
            }
            goto out;
        }

        // Get algorithm digest size
        error = anyrtc_get_sign_algorithm_length(&length, fingerprint->algorithm);
        if (error) {
            if (error == ANYRTC_CODE_UNSUPPORTED_ALGORITHM) {
                continue;
            }
            goto out;
        }

        // Convert hex-encoded value to binary
        length = strlen(fingerprint->value) / 2;
        if (length > sizeof(expected_fingerprint)) {
            DEBUG_WARNING("Hex-encoded fingerprint exceeds buffer size!\n");
            continue;
        }
        err = anyrtc_translate_re_code(str_hex(
                expected_fingerprint, length, fingerprint->value));
        if (err) {
            DEBUG_WARNING("Could not convert hex-encoded fingerprint to binary, reason: %m\n", err);
            continue;
        }

        // Get remote fingerprint
        error = anyrtc_translate_re_code(tls_peer_fingerprint(
                transport->connection, algorithm, actual_fingerprint, sizeof(actual_fingerprint)));
        if (error) {
            goto out;
        }

        // Compare fingerprints
        // TODO: Constant-time equality comparison needed?
        if (memcmp(expected_fingerprint, actual_fingerprint, length) == 0) {
            DEBUG_PRINTF("Peer's certificate fingerprint is valid\n");
            valid = true;
        }
    }

out:
    if (error || !valid) {
        DEBUG_WARNING("Verifying certificate failed, reason: %s\n", anyrtc_code_to_str(error));
        set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_FAILED);

        // Stop
        error = anyrtc_dtls_transport_stop(transport);
        if (error) {
            DEBUG_WARNING("DTLS connection closed, could not stop transport: %s\n",
                          anyrtc_code_to_str(error));
        }
    } else {
        // Connected
        set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_CONNECTED);
    }
}

/*
 * Handle DTLS connection established event.
 */
static void establish_handler(
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;

    // Check state
    if (is_closed(transport)) {
        DEBUG_WARNING("Ignoring established DTLS connection, transport is closed\n");
        return;
    }

    // Connection established
    // Note: State is either 'NEW', 'CONNECTING' or 'FAILED' here
    DEBUG_INFO("DTLS connection established\n");
    transport->connection_established = true;

    // Verify certificate & fingerprint (if remote parameters are available)
    if (transport->remote_parameters) {
        verify_certificate(transport);
    }
}

/*
 * Handle incoming DTLS connection.
 */
static void connect_handler(
        const struct sa* const peer,
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;
    bool role_is_server;
    bool have_connection;
    enum anyrtc_code error;
    int err;

    // Check state
    if (is_closed(transport)) {
        DEBUG_PRINTF("Ignoring incoming DTLS connection, transport is closed\n");
        return;
    }

    // Update role if "auto"
    if (transport->role == ANYRTC_DTLS_ROLE_AUTO) {
        DEBUG_PRINTF("Switching role 'auto' -> 'server'\n");
        transport->role = ANYRTC_DTLS_ROLE_SERVER;
    }
    
    // Accept?
    role_is_server = transport->role == ANYRTC_DTLS_ROLE_SERVER;
    have_connection = transport->connection != NULL;
    if (role_is_server && !have_connection) {
        // Set state to connecting (if not already set)
        if (transport->state != ANYRTC_DTLS_TRANSPORT_STATE_CONNECTING) {
            error = set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_CONNECTING);
            if (error) {
                DEBUG_WARNING("Could not set DTLS transport state to connecting, reason: %s\n",
                              anyrtc_code_to_str(error));
            }
        }

        // Accept and create connection
        DEBUG_PRINTF("Accepting incoming DTLS connection from %J\n", peer);
        err = dtls_accept(&transport->connection, transport->context, transport->socket,
                          establish_handler, dtls_receive_handler, close_handler, transport);
        if (err) {
            DEBUG_WARNING("Could not accept incoming DTLS connection, reason: %m\n", err);
        }
    } else {
        if (have_connection) {
            DEBUG_WARNING("Incoming DTLS connect but we already have a connection\n");
        }
        if (!role_is_server) {
            DEBUG_WARNING("Incoming DTLS connect but role is 'client'\n");
        }
    }
}

/*
 * Initiate a DTLS connect.
 */
static enum anyrtc_code do_connect(
        struct anyrtc_dtls_transport *const transport,
        const struct sa *const peer
) {
    // Connect
    DEBUG_PRINTF("Starting DTLS connection to %J\n", peer);
    return anyrtc_translate_re_code(dtls_connect(
            &transport->connection, transport->context, transport->socket, peer,
            establish_handler, dtls_receive_handler, close_handler, transport));
}

/*
 * Handle outgoing DTLS messages.
 */
static int send_handler(
        struct tls_conn* const tc,
        struct sa const* const original_destination,
        struct mbuf* const buffer,
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;
    struct trice* const ice = transport->ice_transport->gatherer->ice;
    bool closed = is_closed(transport);
    (void) tc;

    // Note: No need to check if closed as only non-application data may be sent if the
    //       transport is already closed.

    // Get selected candidate pair
    struct ice_candpair* const candidate_pair = list_ledata(list_head(trice_validl(ice)));
    if (!candidate_pair) {
        if (!closed) {
            DEBUG_WARNING("Cannot send message, no selected candidate pair\n");
        }
        return ECONNRESET;
    }

    // Get local candidate's UDP socket
    // TODO: What about TCP?
    struct udp_sock* const udp_socket = trice_lcand_sock(ice, candidate_pair->lcand);
    if (!udp_socket) {
        if (!closed) {
            DEBUG_WARNING("Cannot send message, selected candidate pair has no socket\n");
        }
        return ECONNRESET;
    }

    // Send
    // TODO: Is destination correct?
    DEBUG_PRINTF("Sending DTLS message (%zu bytes) to %J (originally: %J) from %J\n",
                 mbuf_get_left(buffer), &candidate_pair->rcand->attr.addr, original_destination,
                 &candidate_pair->lcand->attr.addr);
    int err = udp_send(udp_socket, &candidate_pair->rcand->attr.addr, buffer);
    if (err) {
        DEBUG_WARNING("Could not send, error: %m\n", err);
    }
    return err;
}

/*
 * Handle MTU queries.
 */
static size_t mtu_handler(
        struct tls_conn* const tc,
        void* const arg
) {
    (void) tc; (void) arg;
    // TODO: Choose a sane value.
    return 1400;
}

/*
 * Destructor for an existing ICE transport.
 */
static void anyrtc_dtls_transport_destroy(
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;

    // Remove from ICE transport
    if (transport->ice_transport) {
        transport->ice_transport->dtls_transport = NULL;
    }

    // Dereference
    mem_deref(transport->connection);
    mem_deref(transport->socket);
    mem_deref(transport->context);
    list_flush(&transport->fingerprints);
    list_flush(&transport->candidate_helpers);
    mem_deref(transport->remote_parameters);
    list_flush(&transport->certificates);
    mem_deref(transport->ice_transport);
}

/*
 * Create a new DTLS transport.
 */
enum anyrtc_code anyrtc_dtls_transport_create(
        struct anyrtc_dtls_transport** const transportp, // de-referenced
        struct anyrtc_ice_transport* const ice_transport, // referenced
        struct anyrtc_certificate* const certificates[], // copied (each item)
        size_t const n_certificates,
        anyrtc_dtls_transport_state_change_handler* const state_change_handler, // nullable
        anyrtc_dtls_transport_error_handler* const error_handler, // nullable
        void* const arg // nullable
) {
    struct anyrtc_dtls_transport* transport;
    size_t i;
    struct anyrtc_certificate* certificate;
    enum anyrtc_code error;
    struct le* le;
    uint8_t* certificate_der;
    size_t certificate_der_length;

    // Check arguments
    if (!transportp || !ice_transport || !certificates || !n_certificates) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // TODO: Check certificates expiration dates

    // Check ICE transport state
    if (ice_transport->state == ANYRTC_ICE_TRANSPORT_CLOSED) {
        return ANYRTC_CODE_INVALID_STATE;
    }

    // Check if another DTLS transport is associated to the ICE transport
    if (ice_transport->dtls_transport) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Allocate
    transport = mem_zalloc(sizeof(struct anyrtc_dtls_transport),
                           anyrtc_dtls_transport_destroy);
    if (!transport) {
        return ANYRTC_CODE_NO_MEMORY;
    }

    // Set fields/reference
    transport->state = ANYRTC_DTLS_TRANSPORT_STATE_NEW;
    transport->ice_transport = mem_ref(ice_transport);
    list_init(&transport->certificates);
    transport->state_change_handler = state_change_handler;
    transport->error_handler = error_handler;
    transport->arg = arg;
    transport->role = ANYRTC_DTLS_ROLE_AUTO;
    transport->connection_established = false;
    list_init(&transport->candidate_helpers);
    list_init(&transport->fingerprints);

    // Append and reference certificates
    for (i = 0; i < n_certificates; ++i) {
        // Copy certificate
        // Note: Copying is needed as the 'le' element cannot be associated to multiple lists
        error = anyrtc_certificate_copy(&certificate, certificates[i]);
        if (error) {
            goto out;
        }

        // Append to list
        list_append(&transport->certificates, &certificate->le, certificate);
    }

    // Create (D)TLS context
    DEBUG_PRINTF("Creating DTLS context\n");
    error = anyrtc_translate_re_code(tls_alloc(&transport->context, TLS_METHOD_DTLS, NULL, NULL));
    if (error) {
        goto out;
    }

    // Get DER encoded certificate of choice
    // TODO: Which certificate should we use?
    certificate = list_ledata(list_head(&transport->certificates));
    error = anyrtc_certificate_get_der(
            &certificate_der, &certificate_der_length, certificate, ANYRTC_CERTIFICATE_ENCODE_BOTH);
    if (error) {
        goto out;
    }

    // Set certificate
    DEBUG_PRINTF("Setting certificate on DTLS context\n");
    error = anyrtc_translate_re_code(tls_set_certificate_der(
            transport->context, anyrtc_translate_certificate_key_type(certificate->key_type),
            certificate_der, certificate_der_length, NULL, 0));
    mem_deref(certificate_der);
    if (error) {
        goto out;
    }

    // Set Diffie-Hellman parameters
    // TODO: Get DH params from config
    DEBUG_PRINTF("Setting DH parameters on DTLS context\n");
    error = anyrtc_translate_re_code(tls_set_dh_params_der(transport->context,
            anyrtc_default_dh_parameters, anyrtc_default_dh_parameters_length));
    if (error) {
        goto out;
    }

    // Set cipher suites
    // TODO: Get cipher suites from config
    DEBUG_PRINTF("Setting cipher suites on DTLS context\n");
    error = anyrtc_translate_re_code(tls_set_ciphers(transport->context,
            anyrtc_default_dtls_cipher_suites, anyrtc_default_dtls_cipher_suites_length));
    if (error) {
        goto out;
    }

    // Send client certificate (client) / request client certificate (server)
    tls_set_verify_client(transport->context);

    // Create DTLS socket
    DEBUG_PRINTF("Creating DTLS socket\n");
    error = anyrtc_translate_re_code(dtls_socketless(
            &transport->socket, 0, connect_handler, send_handler, mtu_handler, transport));
    if (error) {
        goto out;
    }

    // Attach to existing candidate pairs
    for (le = list_head(trice_validl(ice_transport->gatherer->ice)); le != NULL; le = le->next) {
        struct ice_candpair* candidate_pair = le->data;
        error = anyrtc_dtls_transport_add_candidate_pair(transport, candidate_pair);
        if (error) {
            DEBUG_WARNING("DTLS transport could not attach to candidate pair, reason: %s\n",
                          anyrtc_code_to_str(error));
        }
    }

    // Attach to ICE transport
    // Note: We cannot reference ourselves here as that would introduce a cyclic reference
    ice_transport->dtls_transport = transport;

out:
    if (error) {
        mem_deref(transport);
    } else {
        // Set pointer
        *transportp = transport;
    }
    return error;
}

/*
 * Handle received UDP messages.
 */
static bool udp_receive_handler(
        struct sa * const source,
        struct mbuf* const buffer,
        void* const arg
) {
    struct anyrtc_dtls_transport* const transport = arg;

    // Decrypt & receive
    // Note: No need to check if the transport is already closed as the messages will re-appear in
    //       the `dtls_receive_handler`.
    dtls_receive(transport->socket, source, buffer);

    // Handled
    return true;
}

/*
 * Destructor for an existing DTLS candidate helper.
 */
static void anyrtc_dtls_candidate_helper_destroy(
        void* const arg
) {
    struct anyrtc_dtls_candidate_helper* const candidate_helper = arg;

    // Dereference
    mem_deref(candidate_helper->helper);
    mem_deref(candidate_helper->candidate);
}

/*
 * Let the DTLS transport attach itself to a candidate pair.
 */
enum anyrtc_code anyrtc_dtls_transport_add_candidate_pair(
        struct anyrtc_dtls_transport* const transport,
        struct ice_candpair* const candidate_pair
) {
    enum anyrtc_code error;
    struct ice_lcand* candidate;
    struct anyrtc_dtls_candidate_helper* candidate_helper;
    
    // Check arguments
    if (!transport || !candidate_pair) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Check state
    if (is_closed(transport)) {
        return ANYRTC_CODE_INVALID_STATE;
    }

    // Get local candidate
    candidate = candidate_pair->lcand;

    // TODO: Check if local candidate is already in our list

    // Get local candidate's UDP socket
    // TODO: What about TCP?
    struct udp_sock* const udp_socket = trice_lcand_sock(
            transport->ice_transport->gatherer->ice, candidate);
    if (!udp_socket) {
        return ANYRTC_CODE_NO_SOCKET;
    }

    // Create DTLS candidate helper
    candidate_helper = mem_zalloc(sizeof(struct anyrtc_dtls_candidate_helper),
                                  anyrtc_dtls_candidate_helper_destroy);
    if (!candidate_helper) {
        return ANYRTC_CODE_NO_MEMORY;
    }

    // Set candidate
    candidate_helper->candidate = mem_ref(candidate);

    // Create & attach UDP helper
    error = anyrtc_translate_re_code(udp_register_helper(
            &candidate_helper->helper, udp_socket, ANYRTC_LAYER_DTLS, NULL,
            udp_receive_handler, transport));
    if (error) {
        goto out;
    }

    // TODO: What about TCP helpers?

    // Do connect (if client and no connection)
    if (transport->role == ANYRTC_DTLS_ROLE_CLIENT && !transport->connection) {
        error = do_connect(transport, &candidate_pair->rcand->attr.addr);
        if (error) {
            goto out;
        }
    }

out:
    if (error) {
        mem_deref(candidate_helper);
    } else {
        // Add to list
        list_append(&transport->candidate_helpers, &candidate_helper->le, candidate_helper);
        DEBUG_PRINTF("Attached DTLS transport to candidate pair\n");
    }
    return error;
}

/*
 * Start the DTLS transport.
 */
enum anyrtc_code anyrtc_dtls_transport_start(
        struct anyrtc_dtls_transport* const transport,
        struct anyrtc_dtls_parameters* const remote_parameters // referenced
) {
    enum anyrtc_code error;
    enum anyrtc_ice_role ice_role;

    // Check arguments
    if (!transport || !remote_parameters) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Validate parameters
    if (!list_head(&remote_parameters->fingerprints)) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Check state
    // Note: Checking for 'ice_remote_parameters' ensures that 'start' is not called twice
    if (transport->remote_parameters || is_closed(transport)) {
        return ANYRTC_CODE_INVALID_STATE;
    }

    // Set state to connecting (if not already set)
    if (transport->state != ANYRTC_DTLS_TRANSPORT_STATE_CONNECTING) {
        error = set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_CONNECTING);
        if (error) {
            return error;
        }
    }

    // Get ICE role
    error = anyrtc_ice_transport_get_role(&ice_role, transport->ice_transport);
    if (error) {
        return error;
    }

    // Determine role
    if (remote_parameters->role == ANYRTC_DTLS_ROLE_AUTO) {
        switch (ice_role) {
            case ANYRTC_ICE_ROLE_CONTROLLED:
                transport->role = ANYRTC_DTLS_ROLE_CLIENT;
                DEBUG_PRINTF("Switching role 'auto' -> 'client'\n");
                break;
            case ANYRTC_ICE_ROLE_CONTROLLING:
                transport->role = ANYRTC_DTLS_ROLE_SERVER;
                DEBUG_PRINTF("Switching role 'auto' -> 'server'\n");
                break;
            default:
                // Cannot continue if ICE transport role is unknown
                DEBUG_WARNING("ICE role must be set before DTLS transport can be started!\n");
                return ANYRTC_CODE_INVALID_STATE;
        }
    } else if (remote_parameters->role == ANYRTC_DTLS_ROLE_SERVER) {
        transport->role = ANYRTC_DTLS_ROLE_CLIENT;
        DEBUG_PRINTF("Switching role 'server' -> 'client'\n");
    } else {
        transport->role = ANYRTC_DTLS_ROLE_SERVER;
        DEBUG_PRINTF("Switching role 'client' -> 'server'\n");
    }

    // Connect (if client)
    if (transport->role == ANYRTC_DTLS_ROLE_CLIENT) {
        // Reset existing connections
        if (transport->connection) {
            // Note: This is needed as ORTC requires us to reset previous DTLS connections
            //       if the remote role is 'server'
            DEBUG_PRINTF("Resetting DTLS connection\n");
            transport->connection = mem_deref(transport->connection);
            transport->connection_established = false;
        }

        // Get selected candidate pair
        struct ice_candpair* const candidate_pair = list_ledata(list_head(trice_validl(
                transport->ice_transport->gatherer->ice)));

        // Do connect (if we have a valid candidate pair)
        if (candidate_pair) {
            error = do_connect(transport, &candidate_pair->rcand->attr.addr);
            if (error) {
                goto out;
            }
        }
    } else {
        // Verify certificate & fingerprint (if connection is established)
        if (transport->connection_established) {
            verify_certificate(transport);
        }
    }

out:
    if (error) {
        transport->connection = mem_deref(transport->connection);
    } else {
        // Set remote parameters
        transport->remote_parameters = mem_ref(remote_parameters);
    }
    return error;
}

/*
 * Stop and close the DTLS transport.
 */
enum anyrtc_code anyrtc_dtls_transport_stop(
        struct anyrtc_dtls_transport* const transport
) {
    // Check arguments
    if (!transport) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Check state
    if (is_closed(transport)) {
        return ANYRTC_CODE_SUCCESS;
    }

    // Update state
    return set_state(transport, ANYRTC_DTLS_TRANSPORT_STATE_CLOSED);

    // TODO: Anything missing?
}

/*
 * Get local DTLS parameters of a transport.
 */
enum anyrtc_code anyrtc_dtls_transport_get_local_parameters(
        struct anyrtc_dtls_parameters** const parametersp, // de-referenced
        struct anyrtc_dtls_transport* const transport
) {
    // TODO: Get config from struct
    enum anyrtc_certificate_sign_algorithm const algorithm = anyrtc_default_config.sign_algorithm;
    struct le* le;
    struct anyrtc_dtls_fingerprint* fingerprint;
    enum anyrtc_code error;

    // Check arguments
    if (!parametersp || !transport) {
        return ANYRTC_CODE_INVALID_ARGUMENT;
    }

    // Check state
    if (is_closed(transport)) {
        return ANYRTC_CODE_INVALID_STATE;
    }

    // Lazy-create fingerprints
    if (!list_head(&transport->fingerprints)) {
        for (le = list_head(&transport->certificates); le != NULL; le = le->next) {
            struct anyrtc_certificate* certificate = le->data;

            // Create fingerprint
            error = anyrtc_dtls_fingerprint_create_empty(&fingerprint, algorithm);
            if (error) {
                return error;
            }

            // Get and set fingerprint of certificate
            error = anyrtc_certificate_get_fingerprint(&fingerprint->value, certificate, algorithm);
            if (error) {
                return error;
            }

            // Append fingerprint
            list_append(&transport->fingerprints, &fingerprint->le, fingerprint);
        }
    }

    // Create and return DTLS parameters instance
    return anyrtc_dtls_parameters_create_internal(
            parametersp, transport->role, &transport->fingerprints);
}
