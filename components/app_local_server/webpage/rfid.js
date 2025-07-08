// RFID Management JavaScript - Simplified Interface with Full Functionality

// Global variable to track current view state
let currentView = 'defaults'; // 'defaults' or 'all'

// Search state management
let searchState = {
    isActive: false,
    type: 'id', // 'id' or 'name'
    query: '',
    allCards: [], // Store all cards for client-side filtering
    filteredCards: []
};

// Admin card protection
const ADMIN_CARD_ID = 305419896; // 0x12345678 in decimal
const ADMIN_CARD_HEX = "0x12345678";

// Initialize page when loaded
$(document).ready(function() {
    loadCardCount();
    loadDefaultCards(); // Load only default cards initially
    initializeSearch(); // Initialize search functionality
});

// Utility function to format card ID for display (hex primary, decimal secondary)
function formatCardId(decimalId) {
    const hex = `0x${decimalId.toString(16).toUpperCase().padStart(8, '0')}`;
    return `
        <div class="card-id-primary">${hex}</div>
        <div class="card-id-secondary">${decimalId}</div>
    `;
}

// Utility function to parse card ID input (supports decimal, hex with/without 0x)
function parseCardId(input) {
    const trimmed = input.toString().trim().toLowerCase();
    
    if (trimmed.startsWith('0x')) {
        // Hex with prefix
        return parseInt(trimmed, 16);
    } else if (/^[0-9a-f]+$/i.test(trimmed) && trimmed.length <= 8 && trimmed.length >= 1) {
        // Could be hex without prefix - check if it's a reasonable hex value
        const hexValue = parseInt(trimmed, 16);
        const decValue = parseInt(trimmed, 10);
        
        // If the input contains letters or is a large number that makes more sense as hex, treat as hex
        if (/[a-f]/i.test(trimmed) || (hexValue <= 0xFFFFFFFF && decValue > 0xFFFFFFFF)) {
            return hexValue;
        } else {
            return decValue;
        }
    } else {
        // Decimal
        return parseInt(trimmed, 10);
    }
}

// Enhanced admin card protection - checks both decimal and hex formats
function isAdminCard(input) {
    const parsedId = parseCardId(input);
    const normalizedInput = input.toString().trim().toLowerCase();
    
    // Check decimal format
    if (parsedId === ADMIN_CARD_ID) return true;
    
    // Check hex format variations
    if (normalizedInput === "0x12345678" || normalizedInput === "12345678") return true;
    
    return false;
}

// Load and display card count
function loadCardCount() {
    $.ajax({
        url: '/cards/count',
        type: 'GET',
        success: function(response) {
            $('#card_count').text(response.card_count || 0);
        },
        error: function() {
            $('#card_count').text('Error loading count');
        }
    });
}

// Load and display default cards only
function loadDefaultCards() {
    currentView = 'defaults'; // Set current view state
    $.ajax({
        url: '/cards/defaults',
        type: 'GET',
        success: function(response) {
            displayCards(response.cards || [], true); // true indicates default cards
            updateViewButton('defaults');
        },
        error: function(xhr, status, error) {
            displayError('cards_tbody', 'Error loading default cards');
        }
    });
}

// Load and display all cards
function loadAllCards() {
    currentView = 'all'; // Set current view state
    $.ajax({
        url: '/cards/get',
        type: 'GET',
        success: function(response) {
            displayCards(response.cards || [], false); // false indicates all cards
            updateViewButton('all');
        },
        error: function(xhr, status, error) {
            displayError('cards_tbody', 'Error loading cards');
        }
    });
}

// Refresh current view - maintains the current view state
function refreshCurrentView() {
    if (currentView === 'defaults') {
        loadDefaultCards();
    } else {
        loadAllCards();
    }
}

// Legacy function for backward compatibility
function loadCards() {
    loadAllCards();
}

// Display cards in table
function displayCards(cards, isDefaultView = false) {
    const tbody = $('#cards_tbody');
    tbody.empty();
    
    if (cards.length === 0) {
        tbody.append('<tr><td colspan="4" style="border: 1px solid #ddd; padding: 8px; text-align: center;">No cards registered</td></tr>');
        return;
    }
    
    cards.forEach(function(card) {
        const timestamp = card.timestamp ? new Date(card.timestamp * 1000).toLocaleDateString() : 'Unknown';
        const status = card.active ? 'Active' : 'Inactive';
        
        // Add visual indicator for default cards
        const defaultBadge = isDefaultView ? '<span style="background: #007bff; color: white; padding: 2px 6px; border-radius: 3px; font-size: 10px; margin-left: 5px;">DEFAULT</span>' : '';
        
        tbody.append(`
            <tr>
                <td style="border: 1px solid #ddd; padding: 8px;">${formatCardId(card.id)}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${card.name}${defaultBadge}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${status}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${timestamp}</td>
            </tr>
        `);
    });
}

// Update the view button text and functionality
function updateViewButton(currentView) {
    const button = $('#view_toggle_btn');
    if (currentView === 'defaults') {
        button.text('View All Cards').attr('onclick', 'loadAllCards()');
    } else {
        button.text('View Defaults Only').attr('onclick', 'loadDefaultCards()');
    }
}

// Add new card
function addCard() {
    const cardIdInput = $('#card_id').val().trim();
    const cardName = $('#card_name').val().trim();
    
    // Clear previous status
    $('#add_status').empty();
    
    // Validation
    if (!cardIdInput || !cardName) {
        showStatus('add_status', 'Please fill in both Card ID and Name', 'error');
        return;
    }
    
    // Parse the card ID (supports decimal, hex with/without 0x)
    const cardId = parseCardId(cardIdInput);
    
    if (isNaN(cardId) || cardId < 1 || cardId > 4294967295) {
        showStatus('add_status', 'Card ID must be between 1 and 4294967295 (0x00000001 to 0xFFFFFFFF)', 'error');
        return;
    }
    
    if (cardName.length > 31) {
        showStatus('add_status', 'Card name must be 31 characters or less', 'error');
        return;
    }
    
    // Send request
    $.ajax({
        url: '/cards/add',
        type: 'POST',
        contentType: 'application/json',
        data: JSON.stringify({
            id: cardId,
            nm: cardName
        }),
        success: function(response, textStatus, xhr) {
            console.log('Add card response:', response, 'Status:', xhr.status);
            // Check for successful HTTP status codes (200, 201)
            if (xhr.status === 200 || xhr.status === 201) {
                showStatus('add_status', 'Card added successfully!', 'success');
                $('#card_id').val('');
                $('#card_name').val('');
                loadCardCount();
                refreshCurrentView(); // Maintain current view instead of switching to all cards
            } else {
                showStatus('add_status', response.message || 'Failed to add card', 'error');
            }
        },
        error: function(xhr, textStatus, errorThrown) {
            console.log('Add card error:', xhr.status, xhr.responseText, textStatus, errorThrown);
            let errorMsg = 'Error adding card';
            try {
                const response = JSON.parse(xhr.responseText);
                errorMsg = response.message || errorMsg;
            } catch (e) {
                errorMsg = `HTTP ${xhr.status}: ${xhr.statusText || errorThrown}`;
            }
            showStatus('add_status', errorMsg, 'error');
        }
    });
}

// Remove card
function removeCard() {
    const cardIdInput = $('#remove_card_id').val().trim();
    
    // Clear previous status
    $('#remove_status').empty();
    
    // Validation
    if (!cardIdInput) {
        showStatus('remove_status', 'Please enter a Card ID to remove', 'error');
        return;
    }
    
    // Parse the card ID (supports decimal, hex with/without 0x)
    const cardId = parseCardId(cardIdInput);
    
    if (isNaN(cardId) || cardId < 1 || cardId > 4294967295) {
        showStatus('remove_status', 'Card ID must be between 1 and 4294967295 (0x00000001 to 0xFFFFFFFF)', 'error');
        return;
    }
    
    // Check if attempting to remove the protected admin card
    if (isAdminCard(cardIdInput)) {
        showStatus('remove_status', `Cannot remove admin card (${ADMIN_CARD_HEX} / ${ADMIN_CARD_ID}) - this card is protected`, 'error');
        return;
    }
    
    // Confirm removal with both hex and decimal display
    const hexDisplay = `0x${cardId.toString(16).toUpperCase().padStart(8, '0')}`;
    if (!confirm(`Are you sure you want to remove card ID ${hexDisplay} (${cardId})?`)) {
        return;
    }
    
    // Send request
    $.ajax({
        url: `/cards/remove?id=${cardId}`,
        type: 'DELETE',
        success: function(response, textStatus, xhr) {
            console.log('Remove card response:', response, 'Status:', xhr.status);
            // Check for successful HTTP status codes (200, 204)
            if (xhr.status === 200 || xhr.status === 204) {
                showStatus('remove_status', 'Card removed successfully!', 'success');
                $('#remove_card_id').val('');
                loadCardCount();
                refreshCurrentView(); // Maintain current view instead of switching to all cards
            } else {
                showStatus('remove_status', response.message || 'Failed to remove card', 'error');
            }
        },
        error: function(xhr, textStatus, errorThrown) {
            console.log('Remove card error:', xhr.status, xhr.responseText, textStatus, errorThrown);
            let errorMsg = 'Error removing card';
            try {
                const response = JSON.parse(xhr.responseText);
                errorMsg = response.message || errorMsg;
            } catch (e) {
                errorMsg = `HTTP ${xhr.status}: ${xhr.statusText || errorThrown}`;
            }
            showStatus('remove_status', errorMsg, 'error');
        }
    });
}

// Reset database
function resetDatabase() {
    if (!confirm('Are you sure you want to reset the RFID database to default cards? This will remove all custom cards and restore the original default cards.')) {
        return;
    }
    
    $.ajax({
        url: '/cards/reset',
        type: 'POST',
        success: function(response, textStatus, xhr) {
            console.log('Reset database response:', response, 'Status:', xhr.status);
            
            // Handle both string and object responses
            let parsedResponse = response;
            if (typeof response === 'string') {
                try {
                    parsedResponse = JSON.parse(response);
                } catch (e) {
                    console.error('Failed to parse response:', response);
                    alert('Failed to reset database: Invalid server response');
                    return;
                }
            }
            
            if (parsedResponse.status === 'success') {
                alert('Database reset to defaults successfully!');
                loadCardCount();
                refreshCurrentView(); // Maintain current view after reset
            } else {
                alert('Failed to reset database: ' + (parsedResponse.message || 'Unknown error'));
            }
        },
        error: function(xhr, textStatus, errorThrown) {
            console.log('Reset database error:', xhr.status, xhr.responseText, textStatus, errorThrown);
            let errorMsg = 'Failed to reset database';
            
            if (xhr.responseText) {
                try {
                    const response = JSON.parse(xhr.responseText);
                    errorMsg = 'Failed to reset database: ' + (response.message || 'Server error');
                } catch (e) {
                    // If JSON parsing fails, use the raw response or status text
                    errorMsg = 'Failed to reset database: ' + (xhr.responseText || xhr.statusText || 'Network error');
                }
            } else {
                errorMsg = 'Failed to reset database: ' + (xhr.statusText || 'Network error');
            }
            
            alert(errorMsg);
        }
    });
}

// Show status message
function showStatus(elementId, message, type) {
    const statusClass = type === 'success' ? 'status-success' : 
                       type === 'error' ? 'status-error' : 'status-info';
    
    $(`#${elementId}`).html(`<div class="status-message ${statusClass}">${message}</div>`);
    
    // Auto-hide success messages after 3 seconds
    if (type === 'success') {
        setTimeout(() => {
            $(`#${elementId}`).empty();
        }, 3000);
    }
}

// Display error in table
function displayError(elementId, message) {
    $(`#${elementId}`).html(`<tr><td colspan="4" style="border: 1px solid #ddd; padding: 8px; text-align: center; color: #721c24;">${message}</td></tr>`);
}

// Handle Enter key in input fields
$(document).on('keypress', '#card_id, #card_name', function(e) {
    if (e.which === 13) { // Enter key
        addCard();
    }
});

$(document).on('keypress', '#remove_card_id', function(e) {
    if (e.which === 13) { // Enter key
        removeCard();
    }
});

// ===== SEARCH FUNCTIONALITY =====

// Initialize search functionality
function initializeSearch() {
    // Update placeholder text based on search type
    updateSearchPlaceholder();
    
    // Add event listeners
    $('#search_type').on('change', function() {
        searchState.type = $(this).val();
        updateSearchPlaceholder();
        performSearch(); // Re-search with new type
    });
    
    // Real-time search with debouncing
    let searchTimeout;
    $('#search_input').on('input', function() {
        clearTimeout(searchTimeout);
        searchTimeout = setTimeout(() => {
            performSearch();
        }, 300); // 300ms debounce
    });
    
    // Handle Enter key in search input
    $('#search_input').on('keypress', function(e) {
        if (e.which === 13) { // Enter key
            clearTimeout(searchTimeout);
            performSearch();
        }
    });
}

// Update search input placeholder based on search type
function updateSearchPlaceholder() {
    const searchInput = $('#search_input');
    if (searchState.type === 'id') {
        searchInput.attr('placeholder', 'Enter Card ID (decimal or hex, e.g., 12345 or 0x3039)...');
    } else {
        searchInput.attr('placeholder', 'Enter name (e.g., John, Doe, or John Doe)...');
    }
}

// Perform search based on current input and type
function performSearch() {
    const query = $('#search_input').val().trim();
    searchState.query = query;
    
    if (query === '') {
        // Empty search - show all cards for current view
        searchState.isActive = false;
        $('#search_status').empty();
        refreshCurrentView();
        return;
    }
    
    searchState.isActive = true;
    
    // Always search the entire database regardless of current view
    $.ajax({
        url: '/cards/get',
        type: 'GET',
        success: function(response) {
            const cards = response.cards || [];
            searchState.allCards = cards;
            
            // Filter cards based on search criteria
            const filteredCards = filterCards(cards, query, searchState.type);
            searchState.filteredCards = filteredCards;
            
            // Display filtered results (always show all matching cards from database)
            displaySearchResults(filteredCards, false);
            
            // Update search status
            updateSearchStatus(filteredCards.length, cards.length, query);
        },
        error: function(xhr, status, error) {
            displayError('cards_tbody', 'Error loading cards for search');
            showStatus('search_status', 'Error performing search', 'error');
        }
    });
}

// Filter cards based on search query and type
function filterCards(cards, query, searchType) {
    if (!query) return cards;
    
    const lowerQuery = query.toLowerCase();
    
    return cards.filter(card => {
        if (searchType === 'id') {
            // ID search - support both decimal and hex formats
            const cardIdStr = card.id.toString();
            const cardIdHex = card.id.toString(16).toLowerCase();
            const cardIdHexPrefixed = '0x' + cardIdHex;
            
            // Parse the query to handle different formats
            let queryId = null;
            try {
                queryId = parseCardId(query);
            } catch (e) {
                // If parsing fails, try string matching
            }
            
            // Exact ID match
            if (queryId !== null && card.id === queryId) {
                return true;
            }
            
            // Partial string matching for ID
            return cardIdStr.includes(lowerQuery) || 
                   cardIdHex.includes(lowerQuery) || 
                   cardIdHexPrefixed.includes(lowerQuery);
        } else {
            // Name search - case-insensitive partial matching
            return card.name.toLowerCase().includes(lowerQuery);
        }
    });
}

// Display search results
function displaySearchResults(cards, isDefaultView = false) {
    const tbody = $('#cards_tbody');
    tbody.empty();
    
    if (cards.length === 0) {
        const searchTypeText = searchState.type === 'id' ? 'Card ID' : 'Name';
        tbody.append(`<tr><td colspan="4" style="border: 1px solid #ddd; padding: 8px; text-align: center;">No cards found matching ${searchTypeText}: "${searchState.query}"</td></tr>`);
        return;
    }
    
    cards.forEach(function(card) {
        const timestamp = card.timestamp ? new Date(card.timestamp * 1000).toLocaleDateString() : 'Unknown';
        const status = card.active ? 'Active' : 'Inactive';
        
        // Add visual indicator for default cards
        const defaultBadge = isDefaultView ? '<span style="background: #007bff; color: white; padding: 2px 6px; border-radius: 3px; font-size: 10px; margin-left: 5px;">DEFAULT</span>' : '';
        
        // Highlight matching text
        let displayName = card.name;
        let displayId = formatCardId(card.id);
        
        if (searchState.type === 'name' && searchState.query) {
            displayName = highlightMatch(card.name, searchState.query);
        } else if (searchState.type === 'id' && searchState.query) {
            displayId = highlightCardIdMatch(card.id, searchState.query);
        }
        
        tbody.append(`
            <tr>
                <td style="border: 1px solid #ddd; padding: 8px;">${displayId}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${displayName}${defaultBadge}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${status}</td>
                <td style="border: 1px solid #ddd; padding: 8px;">${timestamp}</td>
            </tr>
        `);
    });
}

// Highlight matching text in search results
function highlightMatch(text, query) {
    if (!query) return text;
    
    const regex = new RegExp(`(${escapeRegex(query)})`, 'gi');
    return text.replace(regex, '<mark style="background-color: #ffeb3b; padding: 1px 2px;">$1</mark>');
}

// Highlight matching card ID
function highlightCardIdMatch(cardId, query) {
    const hex = `0x${cardId.toString(16).toUpperCase().padStart(8, '0')}`;
    const decimal = cardId.toString();
    
    let highlightedHex = hex;
    let highlightedDecimal = decimal;
    
    if (query) {
        const lowerQuery = query.toLowerCase();
        
        // Try to highlight hex format
        if (hex.toLowerCase().includes(lowerQuery)) {
            highlightedHex = highlightMatch(hex, query);
        }
        
        // Try to highlight decimal format
        if (decimal.includes(lowerQuery)) {
            highlightedDecimal = highlightMatch(decimal, query);
        }
    }
    
    return `
        <div class="card-id-primary">${highlightedHex}</div>
        <div class="card-id-secondary">${highlightedDecimal}</div>
    `;
}

// Escape special regex characters
function escapeRegex(string) {
    return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

// Update search status message
function updateSearchStatus(foundCount, totalCount, query) {
    const searchTypeText = searchState.type === 'id' ? 'Card ID' : 'Name';
    
    if (foundCount === 0) {
        showStatus('search_status', `No cards found matching ${searchTypeText}: "${query}" in database`, 'error');
    } else if (foundCount === totalCount) {
        showStatus('search_status', `Showing all ${totalCount} cards in database`, 'info');
    } else {
        showStatus('search_status', `Found ${foundCount} of ${totalCount} cards in database matching ${searchTypeText}: "${query}"`, 'success');
    }
}

// Clear search and show all cards
function clearSearch() {
    searchState.isActive = false;
    searchState.query = '';
    searchState.allCards = [];
    searchState.filteredCards = [];
    
    $('#search_input').val('');
    $('#search_status').empty();
    
    // Update placeholder for new search type
    updateSearchPlaceholder();
    
    // Refresh current view
    refreshCurrentView();
}

// Override existing functions to maintain search state when needed
const originalRefreshCurrentView = refreshCurrentView;
refreshCurrentView = function() {
    if (searchState.isActive && searchState.query) {
        // If search is active, re-perform search instead of just refreshing
        performSearch();
    } else {
        originalRefreshCurrentView();
    }
};
