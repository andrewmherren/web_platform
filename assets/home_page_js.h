// Home page JavaScript for loading system and modules via API
#ifndef HOME_PAGE_JS_H
#define HOME_PAGE_JS_H

const char HOME_PAGE_JS[] PROGMEM = R"(
// Home Page JavaScript
document.addEventListener('DOMContentLoaded', function() {
    loadHomePageData();
    
    async function loadHomePageData() {
        try {
            // Load system info
            const systemData = await AuthUtils.fetchJSON('/api/system');
            if (systemData.success) {
                updateSystemInfo(systemData.status);
            }
        } catch (error) {
            console.error('Error fetching system data:', error);
        }
        
        try {
            // Load network info
            const networkData = await AuthUtils.fetchJSON('/api/network');
            if (networkData.success) {
                updateNetworkInfo(networkData.network);
            }
        } catch (error) {
            console.error('Error fetching network data:', error);
        }
        
        try {
            // Load modules
            const modulesData = await AuthUtils.fetchJSON('/api/modules');
            if (modulesData.success) {
                updateModuleList(modulesData.modules);
            }
        } catch (error) {
            console.error('Error fetching modules data:', error);
        }
    }
    
    function updateSystemInfo(status) {
        document.getElementById('uptime').textContent = formatUptime(status.uptime);
        document.getElementById('freeMemory').textContent = Math.round(status.memory.freeHeap / 1024);
        document.getElementById('serverProtocol').textContent = status.platform.httpsEnabled ? 'HTTPS (Secure)' : 'HTTP';
        document.getElementById('serverPort').textContent = status.platform.serverPort;
        document.getElementById('hostname').textContent = status.platform.hostname;
    }
    
    function updateNetworkInfo(network) {
        document.getElementById('wifiSsid').textContent = network.ssid;
        document.getElementById('ipAddress').textContent = network.ipAddress;
        document.getElementById('signalStrength').textContent = network.signalStrength;
    }
    
    function updateModuleList(modules) {
        const moduleListContainer = document.getElementById('moduleList');
        moduleListContainer.innerHTML = '';
        
        if (modules.length === 0) {
            moduleListContainer.innerHTML = '<p>No modules registered.</p>';
            return;
        }
        
        modules.forEach(module => {
            const moduleItem = document.createElement('div');
            moduleItem.className = 'module-item';
            
            const moduleInfo = document.createElement('div');
            const moduleName = document.createElement('strong');
            moduleName.textContent = module.name;
            moduleInfo.appendChild(moduleName);
            
            const moduleVersion = document.createElement('small');
            moduleVersion.textContent = ' v' + module.version;
            moduleInfo.appendChild(moduleVersion);
            
            const moduleAction = document.createElement('div');
            const moduleLink = document.createElement('a');
            moduleLink.href = module.basePath;
            moduleLink.className = 'btn btn-secondary';
            moduleLink.textContent = 'Open';
            moduleAction.appendChild(moduleLink);
            
            moduleItem.appendChild(moduleInfo);
            moduleItem.appendChild(moduleAction);
            
            moduleListContainer.appendChild(moduleItem);
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

#endif // HOME_PAGE_JS_H