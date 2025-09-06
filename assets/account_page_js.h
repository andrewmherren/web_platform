#ifndef ACCOUNT_PAGE_JS_H
#define ACCOUNT_PAGE_JS_H

const char ACCOUNT_PAGE_JS[] PROGMEM = R"(
document.addEventListener('DOMContentLoaded', function() {
    loadUserTokens();
    
    // Set up event listeners
    document.getElementById('updatePasswordForm').addEventListener('submit', updatePassword);
    document.getElementById('createTokenForm').addEventListener('submit', createToken);
    
    function loadUserTokens() {
        // First get current user to get the ID
        fetch('/api/user/')
            .then(response => response.json())
            .then(userData => {
                if (userData.success) {
                    const userId = userData.user.id;
                    
                    // Then get the tokens for this user
                    return fetch('/api/users/' + userId + '/tokens');
                }
                throw new Error('Failed to get current user');
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    updateTokensTable(data.tokens);
                }
            })
            .catch(error => {
                console.error('Error loading tokens:', error);
            });
    }
    
    function updateTokensTable(tokens) {
        const tokenContainer = document.getElementById('tokenContainer');
        
        if (!tokens || tokens.length === 0) {
            tokenContainer.innerHTML = '<p>No API tokens have been created yet.</p>';
            return;
        }
        
        let html = '<table class="token-table">';
        html += '<tr><th>Name</th><th>Created</th><th>Actions</th></tr>';
        
        tokens.forEach(token => {
            html += '<tr>';
            html += '<td>' + escapeHtml(token.name) + '</td>';
            html += '<td>' + formatTimestamp(token.createdAt) + '</td>';
            html += '<td><button class="btn btn-danger btn-sm" onclick=deleteToken("' + 
                    token.id + '") > Delete</ button></ td>'; html +=
      '</tr>';
  });

  html += '</table>';
  tokenContainer.innerHTML = html;
  }

  function updatePassword(e) {
    e.preventDefault();
    const password = document.getElementById('password').value;
    const confirmPassword = document.getElementById('confirmPassword').value;

    // Validate passwords match
    if (password !== confirmPassword) {
      showMessage('Passwords do not match', 'error');
      return;
    }

    fetch('/api/user/', {
      method : 'PUT',
      headers : {'Content-Type' : 'application/json'},
      body : JSON.stringify({password : password})
    })
        .then(response => response.json())
        .then(data =>
                    {
                      if (data.success) {
                        showMessage('Password updated successfully', 'success');
                        document.getElementById('password').value = '';
                        document.getElementById('confirmPassword').value = '';
                      } else {
                        showMessage(data.message || 'Failed to update password',
                                    'error');
                      }
                    })
        .catch(error => { showMessage('An error occurred: ' + error, 'error'); });
    }

    function createToken(e) {
      e.preventDefault();
      const tokenName = document.getElementById('tokenName').value;

      // First get current user to get the ID
      fetch('/api/user/')
          .then(response => response.json())
          .then(userData => {
            if (userData.success) {
              const userId = userData.user.id;
              // Then create token for this user
              return fetch('/api/users/' + userId + '/tokens', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({name: tokenName})
              });
            }
            throw new Error('Failed to get current user');
          })
          .then(response => response.json())
          .then(data =>
                      {
                        if (data.success) {
                          UIUtils.showTokenModal(data.token);
                          document.getElementById('tokenName').value = '';
                          loadUserTokens(); // Refresh token list
                        } else {
                          showMessage(data.message || 'Failed to create token',
                                      'error');
                        }
                      })
          .catch(error => { showMessage('An error occurred: ' + error, 'error'); });
    }// Expose the deleteToken function globally
    window.deleteToken = function(tokenId) {
      UIUtils.showConfirm(
        'Delete Token',
        'Are you sure you want to delete this token? This cannot be undone.',
        function() {
          fetch('/api/tokens/' + tokenId, {method: 'DELETE'})
              .then(response => response.json())
              .then(data =>
                          {
                            if (data.success) {
                              showMessage('Token deleted successfully', 'success');
                              loadUserTokens(); // Refresh token list
                            } else {
                              showMessage(data.message || 'Failed to delete token',
                                          'error');
                            }
                          })
              .catch(error => { showMessage('An error occurred: ' + error, 'error'); });
        },
        null
      );
    };

    

    function showMessage(message, type) {
      const messageEl = document.getElementById('statusMessage');
      messageEl.textContent = message;
      messageEl.className = 'alert alert-' + (type || 'info');
      messageEl.style.display = 'block';

      // Hide after 5 seconds
      setTimeout(() => { messageEl.style.display = 'none'; }, 5000);
    }

    function formatTimestamp(timestamp) {
      // Use the platform's TimeUtils for consistent timestamp formatting
      return TimeUtils.formatRelativeTime(timestamp);
    }

    function escapeHtml(text) {
      const div = document.createElement('div');
      div.textContent = text;
      return div.innerHTML;
    }
  });
)";

#endif // ACCOUNT_PAGE_JS_H