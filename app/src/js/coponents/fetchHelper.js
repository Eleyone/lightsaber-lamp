/**
 * Generic fetch helper with unified error handling and notifications
 * Reduces code duplication for API calls with identical error handling patterns
 */
export async function fetchWithNotif(url, options = {}) {
    const method = options.method || 'POST';
    const body = options.body ? JSON.stringify(options.body) : undefined;

    try {
        const response = await fetch(url, {
            method,
            ...(body && { body }),
            headers: {
                ...(body && { 'Content-type': 'application/json; charset=UTF-8' }),
                ...options.headers,
            },
        });

        if (!response.ok) {
            const errorData = await response.json();
            throw new Error(errorData.message || `HTTP Error: ${response.status}`);
        }

        const data = await response.json();
        console.log('Success:', data);

        Alpine.store('notifs').text = data.message;
        Alpine.store('notifs').code = 'success';

        return data;
    } catch (error) {
        console.error('Error:', error);
        Alpine.store('notifs').text = error.message;
        Alpine.store('notifs').code = 'error';
        throw error;
    } finally {
        Alpine.store('notifs').toggle();
    }
}
