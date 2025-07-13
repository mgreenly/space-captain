# OIDC Authentication Integration Research for Space Captain

This document provides an overview of how OpenID Connect (OIDC) authentication could be integrated into the Space Captain CLI client to provide secure user authentication using existing identity providers like Google, Apple, GitHub, and Facebook.

## Overview

OpenID Connect (OIDC) is an identity layer built on top of OAuth 2.0 that enables clients to verify user identity and obtain basic profile information. For CLI applications, OIDC provides a secure, user-friendly authentication method by delegating the login process to the user's web browser and their existing accounts with major providers.

## Authentication Flow

### OAuth 2.0 Authorization Code Flow with PKCE

The recommended approach for CLI applications is the OAuth 2.0 Authorization Code Flow with PKCE (Proof Key for Code Exchange), which provides security for public clients that cannot store client secrets.

### Step-by-Step Flow

1. **Initialization**
   - User runs `spacecaptain login`
   - CLI generates PKCE parameters:
     - `code_verifier`: Random, unguessable string
     - `code_challenge`: SHA-256 hash of code_verifier, Base64URL-encoded
   - CLI starts temporary local web server (e.g., `http://localhost:8910`)

2. **Browser Redirect**
   - CLI constructs authorization URL with:
     - `client_id`: Application's public ID
     - `redirect_uri`: Local server address
     - `response_type=code`
     - `scope=openid profile email`
     - `code_challenge` and `code_challenge_method=S256`
   - Opens URL in user's default browser

3. **User Authentication**
   - User sees familiar provider login screen
   - User authenticates and grants permissions
   - Provider redirects to local callback with authorization code

4. **Token Exchange**
   - CLI exchanges authorization code for tokens
   - Request includes code_verifier for PKCE validation
   - Provider returns:
     - `access_token`: For API calls
     - `refresh_token`: For token renewal
     - `id_token`: JWT with user identity (OIDC providers only)

5. **Token Storage**
   - Tokens stored securely in OS credential store
   - User identity extracted from id_token

## User Identity Management

### ID Token Structure
```json
{
  "iss": "https://accounts.google.com",
  "sub": "112345678901234567890",
  "email": "user@example.com",
  "email_verified": true,
  "name": "User Name",
  "iat": 1618231022,
  "exp": 1618234622
}
```

### Unique User Identification
- Use combination of `iss` (issuer) and `sub` (subject) claims
- Example: `google_112345678901234567890`
- Ensures unique identification across providers

## Implementation Architecture

### Required Components

1. **OIDC Client Library**
   - Handles protocol complexity
   - Manages PKCE flow
   - Token validation and refresh

2. **Local Callback Server**
   - Temporary HTTP server for redirect handling
   - Runs on random available port
   - Shows success message and closes

3. **Secure Token Storage**
   - OS-specific credential stores:
     - macOS: Keychain
     - Windows: Credential Manager
     - Linux: Secret Service API/GNOME Keyring
   - Never store tokens in plain text

4. **Browser Integration**
   - Cross-platform browser opening
   - Fallback to manual URL display

### Configuration
```c
// OIDC Configuration
#define OIDC_CLIENT_ID "spacecaptain-cli"
#define OIDC_REDIRECT_URI "http://localhost:8910/callback"
#define OIDC_SCOPES "openid profile email"
#define OIDC_RESPONSE_TYPE "code"
#define OIDC_GRANT_TYPE "authorization_code"
```

## Provider Integration

### Google (Full OIDC)
- Standard OIDC implementation
- Complete id_token support
- Well-documented flow
- Discovery endpoint: `https://accounts.google.com/.well-known/openid-configuration`

### Apple (Full OIDC)
- Sign in with Apple
- Privacy features (email hiding)
- Requires Apple Developer account
- Discovery endpoint: `https://appleid.apple.com/.well-known/openid-configuration`

### GitHub (OAuth 2.0 Only)
- No id_token support
- Requires additional API call to `/user` endpoint
- Use GitHub user ID as stable identifier
- Authorization URL: `https://github.com/login/oauth/authorize`
- Token URL: `https://github.com/login/oauth/access_token`

### Facebook (OAuth 2.0 Only)
- No id_token support
- Requires Graph API call for user info
- Use Facebook user ID as stable identifier
- Authorization URL: `https://www.facebook.com/v12.0/dialog/oauth`

## Security Considerations

### Token Security
1. **Never store tokens in plain text**
2. **Use OS credential stores exclusively**
3. **Implement token rotation**
4. **Clear tokens on logout**

### PKCE Requirements
1. **Generate cryptographically secure verifier**
2. **Use SHA256 for challenge**
3. **Verify state parameter**
4. **Validate redirect URI**

### Network Security
1. **Always use HTTPS for provider communication**
2. **Validate TLS certificates**
3. **Implement request timeouts**
4. **Handle network errors gracefully**

## Implementation Recommendations

### Recommended Libraries

| Language | Libraries |
|----------|-----------|
| C | libcurl + custom OIDC handling |
| Go | `golang.org/x/oauth2`, `github.com/coreos/go-oidc` |
| Python | `Authlib` |
| Node.js | `openid-client` |
| Rust | `openidconnect-rs` |

### Error Handling
1. **Network failures**: Retry with exponential backoff
2. **Invalid tokens**: Trigger re-authentication
3. **User cancellation**: Graceful exit
4. **Provider errors**: Clear error messages

### User Experience
1. **Clear login prompts**
2. **Success confirmation in browser**
3. **Automatic token refresh**
4. **Session persistence across commands**

## Testing Strategy

### Unit Testing
1. PKCE generation
2. Token parsing
3. Secure storage mock
4. Error handling

### Integration Testing
1. Full authentication flow
2. Token refresh flow
3. Multiple provider support
4. Network failure scenarios

### Security Testing
1. Token storage security
2. PKCE validation
3. State parameter checking
4. Redirect URI validation

## Implementation Phases

### Phase 1: Basic OIDC Support
- Single provider (Google)
- Manual token refresh
- Basic error handling

### Phase 2: Multi-Provider Support
- Add GitHub, Apple support
- Automatic token refresh
- Provider abstraction layer

### Phase 3: Enhanced Security
- Implement PKCE
- Add state parameter
- Enhanced error handling

### Phase 4: Production Features
- Multiple account support
- Account switching
- Logout functionality
- Token revocation

## Conclusion

OIDC integration provides a secure, user-friendly authentication method for the Space Captain CLI client. By leveraging existing identity providers and following security best practices, users can authenticate without managing separate credentials while maintaining security through proper token handling and storage.