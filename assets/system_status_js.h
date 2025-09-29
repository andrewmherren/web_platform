// System status JavaScript for loading system information via API
// This file will be included as an asset in the HTML page

#ifndef SYSTEM_STATUS_JS_H
#define SYSTEM_STATUS_JS_H

const char SYSTEM_STATUS_JS[] PROGMEM = R"(
// System Status JavaScript
document.addEventListener('DOMContentLoaded', function() {
    loadSystemData();
    
    // Refresh data every 5 seconds
    setInterval(loadSystemData, 5000);
    
    async function loadSystemData() {
        try {
            // Load system status data
            const systemData = await AuthUtils.fetchJSON('/api/system');
            if (systemData.success) {
                updateSystemStatus(systemData.status);
            }
        } catch (error) {
            console.error('Error fetching system data:', error);
        }
        
        try {
            // Load network status data
            const networkData = await AuthUtils.fetchJSON('/api/network');
            if (networkData.success) {
                updateNetworkStatus(networkData.network);
            }
        } catch (error) {
            console.error('Error fetching network data:', error);
        }
        
        try {
            // Load modules data
            const modulesData = await AuthUtils.fetchJSON('/api/modules');
            if (modulesData.success) {
                updateModulesTable(modulesData.modules);
            }
        } catch (error) {
            console.error('Error fetching modules data:', error);
        }
    }
    
    function updateSystemStatus(status) {
        // Memory gauge
        document.getElementById('uptimeValue').textContent = formatUptime(status.uptime);
        document.getElementById('freeHeap').textContent = status.memory.freeHeap;
        document.getElementById('freeHeapPercent').textContent = status.memory.freeHeapPercent;
        document.getElementById('memory-gauge-fill').style.width = (100 - status.memory.freeHeapPercent) + '%';
        document.getElementById('memory-gauge').className = 'gauge gauge-' + status.memory.color;
        
        // Storage gauge
        document.getElementById('flashSize').textContent = status.storage.flashSize + ' MB';
        document.getElementById('freeSpace').textContent = status.storage.availableSpace;
        document.getElementById('freeSpacePercent').textContent = (100 - status.storage.usedSpacePercent);
        document.getElementById('storage-gauge-fill').style.width = status.storage.usedSpacePercent + '%';
        document.getElementById('storage-gauge').className = 'gauge gauge-' + status.storage.color;
        
        // Platform info
        document.getElementById('platformMode').textContent = status.platform.mode;
        document.getElementById('httpsStatus').textContent = status.platform.httpsEnabled ? 'Enabled' : 'Disabled';
        document.getElementById('serverPort').textContent = status.platform.serverPort;
        document.getElementById('hostname').textContent = status.platform.hostname;
        document.getElementById('moduleCount').textContent = status.platform.moduleCount;
        document.getElementById('routeCount').textContent = status.platform.routeCount;
        document.getElementById('systemVersion').textContent = status.platform.systemVersion;
        document.getElementById('platformVersion').textContent = status.platform.platformVersion;
    }
    
    function updateNetworkStatus(network) {
        document.getElementById('wifiSsid').textContent = network.ssid;
        document.getElementById('ipAddress').textContent = network.ipAddress;
        document.getElementById('macAddress').textContent = network.macAddress;
        document.getElementById('signalStrength').textContent = network.signalStrength;
    }
    
    function updateModulesTable(modules) {
        const tableBody = document.getElementById('modulesTableBody');
        tableBody.innerHTML = '';
        
        if (modules.length === 0) {
            const row = document.createElement('tr');
            const cell = document.createElement('td');
            cell.colSpan = 3;
            cell.textContent = 'No modules registered.';
            row.appendChild(cell);
            tableBody.appendChild(row);
            return;
        }
        
        modules.forEach(module => {
            const row = document.createElement('tr');
            
            const nameCell = document.createElement('td');
            nameCell.textContent = module.name;
            row.appendChild(nameCell);
            
            const versionCell = document.createElement('td');
            versionCell.textContent = module.version;
            row.appendChild(versionCell);
            
            const pathCell = document.createElement('td');
            pathCell.textContent = module.basePath;
            row.appendChild(pathCell);
            
            tableBody.appendChild(row);
        });
    }
    
    function formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        seconds %= 86400;
        const hours = Math.floor(seconds / 3600);
        seconds %= 3600;
        const minutes = Math.floor(seconds / 60);
        seconds %= 60;
        
        let result = '';
        if (days > 0) result += days + 'd ';
        if (hours > 0 || days > 0) result += hours + 'h ';
        if (minutes > 0 || hours > 0 || days > 0) result += minutes + 'm ';
        result += seconds + 's';
        
        return result;
    }
});
)";

#endif // SYSTEM_STATUS_JS_H