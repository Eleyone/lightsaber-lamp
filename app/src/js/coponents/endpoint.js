export default {
    saveWifiConfig($formData) {
        fetch('/wifi-settings', {
            method: 'POST',
            body: JSON.stringify($formData),
            headers: {
                'Content-type': 'application/json; charset=UTF-8',
            }
        }).then((response) => {
            if (response.ok) {
                return response.json()
            } else {
                let json = response.json();

                throw new Error(json.message, response)
            }
        }).then((message) => {
            console.log('Success: ', message);
            Alpine.store('notifs').text = message.message;
            Alpine.store('notifs').code = 'success';
        }).catch((error) => {
            console.error('Error:', error);
            Alpine.store('notifs').text = error.message;
            Alpine.store('notifs').code = 'error';
        }).finally(() => {
            Alpine.store('notifs').toggle();
        });
    },
    resetWifiConfig() {
        fetch('/reset-wifi-settings', {
            method: 'POST',
        }).then((response) => {
            if (response.ok) {
                return response.json()
            } else {
                let json = response.json();
                throw new Error(json.message, response)
            }
        }).then((message) => {
            console.log('Success: ', message);
            Alpine.store('notifs').text = message.message;
            Alpine.store('notifs').code = 'success';
        }).catch((error) => {
            console.error('Error:', error);
            Alpine.store('notifs').text = error.message;
            Alpine.store('notifs').code = 'error';
        }).finally(() => {
            Alpine.store('notifs').toggle();
        });
    },
    saveNetworkConfig($formData) {
        fetch('/network-settings', {
            method: 'POST',
            body: JSON.stringify($formData),
            headers: {
                'Content-type': 'application/json; charset=UTF-8',
            }
        }).then((response) => {
            if (response.ok) {
                return response.json()
            } else {
                let json = response.json();

                throw new Error(json.message, response)
            }
        }).then((message) => {
            console.log('Success: ', message);
            Alpine.store('notifs').text = message.message;
            Alpine.store('notifs').code = 'success';
        }).catch((error) => {
            console.error('Error:', error);
            Alpine.store('notifs').text = error.message;
            Alpine.store('notifs').code = 'error';
        }).finally(() => {
            Alpine.store('notifs').toggle();
        });
    },
    resetNetworkConfig() {
        fetch('/reset-network-settings', {
            method: 'POST',
        }).then((response) => {
            if (response.ok) {
                return response.json()
            } else {
                let json = response.json();
                throw new Error(json.message, response)
            }
        }).then((message) => {
            console.log('Success: ', message);
            Alpine.store('notifs').text = message.message;
            Alpine.store('notifs').code = 'success';
        }).catch((error) => {
            console.error('Error:', error);
            Alpine.store('notifs').text = error.message;
            Alpine.store('notifs').code = 'error';
        }).finally(() => {
            Alpine.store('notifs').toggle();
        });
    }
}