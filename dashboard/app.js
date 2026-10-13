/**
 * ============================================================================
 * LINUX MEMORY-MAPPED FILE MANAGEMENT SYSTEM - CLIENT CONTROLLER (app.js)
 * Real-time State Synchronization, System Call Triggers, and Visualizations
 * ============================================================================
 */

document.addEventListener('DOMContentLoaded', () => {
  // Application State Cache
  let currentState = {
    is_file_open: false,
    is_mapped: false,
    is_synced: true,
    fd: -1,
    filename: '',
    mapped_address: null,
    mapped_size: 0,
    file_size: 0,
    op_count: 0,
    read_count: 0,
    write_count: 0,
    sync_count: 0,
    proc_maps: ''
  };

  let pollInterval = null;

  // DOM Elements - Metrics
  const cardFilename = document.getElementById('card-filename');
  const cardFileDesc = document.getElementById('card-file-desc');
  const cardFd = document.getElementById('card-fd');
  const cardFilesize = document.getElementById('card-filesize');
  const cardBlocks = document.getElementById('card-blocks');
  const cardAddress = document.getElementById('card-address');
  const cardMapSize = document.getElementById('card-map-size');
  const badgeMapStatus = document.getElementById('badge-map-status');
  const badgeSyncStatus = document.getElementById('badge-sync-status');
  const cardSyncSubtext = document.getElementById('card-sync-subtext');
  const cardPagesize = document.getElementById('card-pagesize');
  const cardPageCount = document.getElementById('card-page-count');
  const headerMapPill = document.getElementById('header-map-pill');
  const vmaEntryText = document.getElementById('vma-entry-text');
  const vmaActiveLabel = document.getElementById('vma-active-label');

  // DOM Elements - Interactive Memory Flow Diagram
  const nodeFile = document.getElementById('node-file');
  const nodeFd = document.getElementById('node-fd');
  const nodeMemory = document.getElementById('node-memory');
  const nodeAccess = document.getElementById('node-access');
  const nodeSync = document.getElementById('node-sync');
  const nodeTeardown = document.getElementById('node-teardown');
  const flowFileName = document.getElementById('flow-file-name');
  const flowFdVal = document.getElementById('flow-fd-val');
  const flowMemAddr = document.getElementById('flow-mem-addr');
  const flowDirtyStatus = document.getElementById('flow-dirty-status');

  // DOM Elements - Content Viewer
  const mappedTextDisplay = document.getElementById('mapped-text-display');
  const hexTableBody = document.getElementById('hex-table-body');
  const writeOffsetInput = document.getElementById('write-offset');
  const writeTextInput = document.getElementById('write-text');
  const writeGuardBadge = document.getElementById('write-guard-badge');

  // DOM Elements - Terminal Logs
  const terminalLogs = document.getElementById('terminal-logs');
  const fullTerminalLogs = document.getElementById('full-terminal-logs');

  // DOM Elements - Modals & Alerts
  const alertBanner = document.getElementById('alert-banner');
  const alertMessage = document.getElementById('alert-message');
  const alertClose = document.getElementById('alert-close');
  const toastContainer = document.getElementById('toast-container');

  // Initialize
  initNavigation();
  initModals();
  initEventHandlers();
  fetchStatus();
  fetchLogs();

  // Polling loop (every 1500ms)
  pollInterval = setInterval(() => {
    fetchStatus();
    fetchLogs();
  }, 1500);

  // ==========================================================================
  // API Calls & State Management
  // ==========================================================================

  async function fetchStatus() {
    try {
      const res = await fetch('/api/status');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      if (data.success && data.state) {
        updateUI(data.state, data.system);
      }
    } catch (err) {
      console.warn('Status fetch error:', err);
    }
  }

  async function fetchLogs() {
    try {
      const res = await fetch('/api/logs');
      if (!res.ok) return;
      const data = await res.json();
      if (data.success && Array.isArray(data.logs)) {
        renderLogs(data.logs);
      }
    } catch (err) {
      console.warn('Logs fetch error:', err);
    }
  }

  async function fetchMappedContent() {
    if (!currentState.is_mapped) {
      mappedTextDisplay.textContent = 'No file is currently mapped in memory. Open and Map a file first.';
      hexTableBody.innerHTML = '<tr><td colspan="4" class="text-center text-muted">No memory mapped</td></tr>';
      return;
    }

    try {
      const res = await fetch('/api/read');
      const data = await res.json();
      if (data.success) {
        mappedTextDisplay.textContent = data.text || '[Empty mapping or binary contents]';
        renderHexTable(data.hex_rows);
      } else {
        mappedTextDisplay.textContent = `[Read error: ${data.error}]`;
      }
    } catch (err) {
      console.error('Content read error:', err);
    }
  }

  // ==========================================================================
  // UI Rendering & State Sync
  // ==========================================================================

  function updateUI(st, sys) {
    const prevMapped = currentState.is_mapped;
    const prevSynced = currentState.is_synced;
    currentState = st;

    // Card: Filename & FD
    if (st.is_file_open) {
      cardFilename.textContent = st.filename || 'Untitled';
      cardFilename.title = st.filename;
      cardFd.textContent = st.fd;
      cardFileDesc.innerHTML = `Descriptor: <strong>FD ${st.fd}</strong>`;
      flowFileName.textContent = st.filename.split('/').pop();
      flowFdVal.textContent = `FD: ${st.fd}`;
    } else {
      cardFilename.textContent = 'No File Open';
      cardFilename.title = 'No file open';
      cardFd.textContent = '-1';
      cardFileDesc.innerHTML = 'Descriptor: <strong>Closed (-1)</strong>';
      flowFileName.textContent = 'Disk Storage';
      flowFdVal.textContent = 'FD: None';
    }

    // Card: File Size
    cardFilesize.textContent = formatBytes(st.file_size);
    const blocks = Math.ceil(st.file_size / 512);
    cardBlocks.textContent = `Blocks: ${blocks} (512B)`;

    // Card: Address & Mapped Size
    if (st.is_mapped && st.mapped_address) {
      cardAddress.textContent = st.mapped_address;
      cardMapSize.innerHTML = `Mapped Size: <strong>${formatBytes(st.mapped_size)}</strong>`;
      flowMemAddr.textContent = st.mapped_address;
      headerMapPill.textContent = 'MAPPED';
      headerMapPill.className = 'status-pill status-mapped';
      badgeMapStatus.textContent = 'MAPPED';
      badgeMapStatus.className = 'badge badge-mapped';
      writeGuardBadge.textContent = `Active at ${st.mapped_address}`;
      writeGuardBadge.className = 'badge badge-mapped';
      vmaActiveLabel.textContent = `Current: ${st.mapped_address} (${formatBytes(st.mapped_size)})`;
    } else {
      cardAddress.textContent = 'NULL (Unmapped)';
      cardMapSize.innerHTML = 'Mapped Size: <strong>0 B</strong>';
      flowMemAddr.textContent = 'Virtual Addr';
      headerMapPill.textContent = 'UNMAPPED';
      headerMapPill.className = 'status-pill status-unmapped';
      badgeMapStatus.textContent = 'UNMAPPED';
      badgeMapStatus.className = 'badge badge-unmapped';
      writeGuardBadge.textContent = 'Requires Active mmap()';
      writeGuardBadge.className = 'badge badge-warning';
      vmaActiveLabel.textContent = 'No Active Mapping';
    }

    // Card: Sync Status
    if (st.is_synced) {
      badgeSyncStatus.textContent = 'SYNCED';
      badgeSyncStatus.className = 'badge badge-synced';
      cardSyncSubtext.textContent = 'Pages Match Physical Disk';
      flowDirtyStatus.textContent = 'Clean / In-Sync';
    } else {
      badgeSyncStatus.textContent = 'PENDING';
      badgeSyncStatus.className = 'badge badge-pending';
      cardSyncSubtext.textContent = 'Dirty Pages Pending msync()';
      flowDirtyStatus.textContent = 'Dirty Pages Modified';
    }

    // Card: Page Size & Calculation
    const pageSize = (sys && sys.page_size) || 4096;
    cardPagesize.textContent = `${pageSize} B`;
    const pageFrames = st.file_size > 0 ? Math.ceil(st.file_size / pageSize) : 0;
    cardPageCount.textContent = `${pageFrames} Page Frame${pageFrames === 1 ? '' : 's'} (${pageFrames * pageSize} B)`;

    // VMA /proc/self/maps Bar
    if (st.proc_maps && st.proc_maps !== 'No active memory mapping') {
      vmaEntryText.textContent = st.proc_maps;
      vmaEntryText.style.color = '#38bdf8';
    } else {
      vmaEntryText.textContent = 'No active mapping in process virtual address space';
      vmaEntryText.style.color = '#94a3b8';
    }

    // System Info Table
    if (sys) {
      const sysKernel = document.getElementById('sys-kernel');
      const sysOs = document.getElementById('sys-os');
      const sysWsl = document.getElementById('sys-wsl');
      const sysArch = document.getElementById('sys-arch');
      const sysPageSize = document.getElementById('sys-page-size');
      const sysPid = document.getElementById('sys-pid');
      const sysPyVer = document.getElementById('sys-py-ver');
      const sidebarKernel = document.getElementById('sidebar-kernel');
      const sidebarOsStatus = document.getElementById('sidebar-os-status');

      if (sysKernel) sysKernel.textContent = sys.kernel || 'Linux 6.x';
      if (sysOs) sysOs.textContent = `${sys.distro || 'Linux'} (${sys.os_name || 'POSIX'})`;
      if (sysWsl) sysWsl.innerHTML = sys.is_wsl ? '<span class="badge badge-mapped">WSL2 Detected</span>' : '<span class="badge badge-outline">Native Linux</span>';
      if (sysArch) sysArch.textContent = sys.machine || 'x86_64';
      if (sysPageSize) sysPageSize.textContent = `${sys.page_size || 4096} Bytes (4 KB)`;
      if (sysPid) sysPid.textContent = `${sys.pid || '-'}`;
      if (sysPyVer) sysPyVer.textContent = `Python ${sys.python_version || '3.x'} + ctypes standard ABI`;
      if (sidebarKernel) sidebarKernel.textContent = `Kernel ${sys.kernel ? sys.kernel.split('-')[0] : '6.x'} POSIX`;
      if (sidebarOsStatus) sidebarOsStatus.textContent = sys.is_wsl ? 'WSL2 / Linux Ready' : 'Linux Ready';
    }

    // Highlight Flow Diagram Nodes
    updateFlowDiagram(st);

    // Refresh content viewer if mapping or sync changed
    if (st.is_mapped && (!prevMapped || prevSynced !== st.is_synced)) {
      fetchMappedContent();
    } else if (!st.is_mapped && prevMapped) {
      mappedTextDisplay.textContent = 'No file is currently mapped in memory. Open and Map a file first.';
      hexTableBody.innerHTML = '<tr><td colspan="4" class="text-center text-muted">No memory mapped</td></tr>';
    }
  }

  function updateFlowDiagram(st) {
    // Reset all nodes
    [nodeFile, nodeFd, nodeMemory, nodeAccess, nodeSync, nodeTeardown].forEach(n => {
      n.classList.remove('active');
    });

    if (!st.is_file_open) {
      nodeFile.classList.add('active');
    } else if (st.is_file_open && !st.is_mapped) {
      nodeFd.classList.add('active');
    } else if (st.is_mapped && st.is_synced) {
      nodeMemory.classList.add('active');
    } else if (st.is_mapped && !st.is_synced) {
      nodeAccess.classList.add('active');
    }
  }

  function renderHexTable(rows) {
    if (!rows || rows.length === 0) {
      hexTableBody.innerHTML = '<tr><td colspan="4" class="text-center text-muted">No bytes to display</td></tr>';
      return;
    }

    let html = '';
    rows.forEach(r => {
      html += `<tr>
        <td>${escapeHtml(r.offset)}</td>
        <td>${escapeHtml(r.address)}</td>
        <td>${escapeHtml(r.hex)}</td>
        <td>${escapeHtml(r.ascii)}</td>
      </tr>`;
    });
    hexTableBody.innerHTML = html;
  }

  function renderLogs(logs) {
    if (!logs) return;

    const formatLogLine = l => {
      let levelClass = 'log-info';
      if (l.level === 'SUCCESS') levelClass = 'log-success';
      else if (l.level === 'ERROR') levelClass = 'log-error';
      else if (l.level === 'WARN') levelClass = 'log-warn';

      const syscallTag = l.syscall ? ` <span style="color:#a78bfa;">[${escapeHtml(l.syscall)}]</span>` : '';
      return `<div class="log-line ${levelClass}">[${escapeHtml(l.timestamp)}] [${escapeHtml(l.level)}]${syscallTag} ${escapeHtml(l.message)}</div>`;
    };

    // Terminal preview on dashboard
    terminalLogs.innerHTML = logs.slice(-25).map(formatLogLine).join('');
    terminalLogs.scrollTop = terminalLogs.scrollHeight;

    // Full terminal on Activity Logs tab
    if (fullTerminalLogs) {
      fullTerminalLogs.innerHTML = logs.map(formatLogLine).join('');
      fullTerminalLogs.scrollTop = fullTerminalLogs.scrollHeight;
    }
  }

  // ==========================================================================
  // User Actions & Handlers
  // ==========================================================================

  function initEventHandlers() {
    // 1. Open File
    document.getElementById('action-btn-open').addEventListener('click', () => {
      showModal('modal-open');
    });

    document.getElementById('btn-modal-confirm-open').addEventListener('click', async () => {
      const filename = document.getElementById('modal-input-filename').value.trim();
      if (!filename) {
        showToast('Filename cannot be empty', 'error');
        return;
      }
      hideModal('modal-open');
      await executeApiCall('/api/open', { filename });
    });

    // 2. Create File
    document.getElementById('action-btn-create').addEventListener('click', () => {
      showModal('modal-create');
    });

    document.getElementById('btn-modal-confirm-create').addEventListener('click', async () => {
      const filename = document.getElementById('modal-create-name').value.trim();
      const initial_data = document.getElementById('modal-create-content').value;
      if (!filename) {
        showToast('Filename cannot be empty', 'error');
        return;
      }
      hideModal('modal-create');
      await executeApiCall('/api/create', { filename, initial_data });
    });

    // 3. Map File
    document.getElementById('action-btn-map').addEventListener('click', async () => {
      if (!currentState.is_file_open) {
        showToast('Please open or create a file first before calling mmap()', 'error');
        return;
      }
      await executeApiCall('/api/map', {});
      await fetchMappedContent();
    });

    // 4. Read Data
    document.getElementById('action-btn-read').addEventListener('click', async () => {
      if (!currentState.is_mapped) {
        showToast('No file is mapped in memory. Call mmap() first.', 'error');
        return;
      }
      await fetchMappedContent();
      showToast('Mapped data refreshed from virtual memory pointer', 'info');
    });

    // 5. Write Data (Form)
    document.getElementById('memory-write-form').addEventListener('submit', async e => {
      e.preventDefault();
      if (!currentState.is_mapped) {
        showToast('No active memory mapping. Call mmap() first.', 'error');
        return;
      }
      const offset = parseInt(writeOffsetInput.value, 10) || 0;
      const data = writeTextInput.value;
      if (!data) {
        showToast('Please enter text to write into mapped memory', 'error');
        return;
      }
      await executeApiCall('/api/write', { offset, data });
      await fetchMappedContent();
    });

    document.getElementById('action-btn-write').addEventListener('click', () => {
      writeTextInput.focus();
    });

    // 6. Sync Changes (msync)
    document.getElementById('action-btn-sync').addEventListener('click', async () => {
      if (!currentState.is_mapped) {
        showToast('No active memory mapping to sync', 'error');
        return;
      }
      await executeApiCall('/api/sync', {});
      await fetchMappedContent();
    });

    document.getElementById('btn-quick-sync').addEventListener('click', async () => {
      if (!currentState.is_mapped) {
        showToast('No active memory mapping to sync', 'error');
        return;
      }
      await executeApiCall('/api/sync', {});
      await fetchMappedContent();
    });

    // 7. Unmap Memory (munmap)
    document.getElementById('action-btn-unmap').addEventListener('click', async () => {
      if (!currentState.is_mapped) {
        showToast('No active memory mapping to unmap', 'error');
        return;
      }
      await executeApiCall('/api/unmap', {});
    });

    document.getElementById('btn-direct-unmap').addEventListener('click', async () => {
      if (!currentState.is_mapped) {
        showToast('No active memory mapping to unmap', 'error');
        return;
      }
      await executeApiCall('/api/unmap', {});
    });

    // 8. Close File (close)
    document.getElementById('action-btn-close').addEventListener('click', async () => {
      if (!currentState.is_file_open) {
        showToast('No file is currently open', 'error');
        return;
      }
      await executeApiCall('/api/close', {});
    });

    document.getElementById('btn-direct-close').addEventListener('click', async () => {
      if (!currentState.is_file_open) {
        showToast('No file is currently open', 'error');
        return;
      }
      await executeApiCall('/api/close', {});
    });

    // 9. Automated Demo Walkthrough
    document.getElementById('btn-run-demo').addEventListener('click', async () => {
      showToast('Starting automated 10-step mmap lifecycle demo...', 'info');
      await executeApiCall('/api/demo', {});
      await fetchMappedContent();
    });

    // 10. Forms in File Operations tab
    document.getElementById('form-open-file').addEventListener('submit', async e => {
      e.preventDefault();
      const filename = document.getElementById('input-open-path').value.trim();
      if (filename) await executeApiCall('/api/open', { filename });
    });

    document.getElementById('form-create-file').addEventListener('submit', async e => {
      e.preventDefault();
      const filename = document.getElementById('input-create-path').value.trim();
      const initial_data = document.getElementById('input-create-content').value;
      if (filename) await executeApiCall('/api/create', { filename, initial_data });
    });

    document.getElementById('form-resize-file').addEventListener('submit', async e => {
      e.preventDefault();
      const size = parseInt(document.getElementById('input-resize-size').value, 10);
      if (size > 0) await executeApiCall('/api/resize', { size });
    });

    // 11. Refresh Content button
    document.getElementById('btn-refresh-content').addEventListener('click', async () => {
      await fetchMappedContent();
      showToast('Memory view refreshed', 'info');
    });

    // 12. Clear Logs
    document.getElementById('btn-clear-logs').addEventListener('click', async () => {
      await fetch('/api/logs/clear', { method: 'POST' });
      terminalLogs.innerHTML = '';
      if (fullTerminalLogs) fullTerminalLogs.innerHTML = '';
      showToast('Logs cleared', 'info');
    });

    const btnClearTab = document.getElementById('btn-clear-logs-tab');
    if (btnClearTab) {
      btnClearTab.addEventListener('click', async () => {
        await fetch('/api/logs/clear', { method: 'POST' });
        terminalLogs.innerHTML = '';
        if (fullTerminalLogs) fullTerminalLogs.innerHTML = '';
      });
    }

    // 13. Copy Logs
    document.getElementById('btn-copy-logs').addEventListener('click', () => {
      const text = terminalLogs.innerText;
      navigator.clipboard.writeText(text);
      showToast('Logs copied to clipboard', 'info');
    });

    // 14. Quick Sample Chips
    document.querySelectorAll('.sample-chips .chip').forEach(chip => {
      chip.addEventListener('click', async () => {
        const file = chip.getAttribute('data-file');
        await executeApiCall('/api/open', { filename: file });
      });
    });

    document.querySelectorAll('.chip-modal').forEach(chip => {
      chip.addEventListener('click', () => {
        const file = chip.getAttribute('data-file');
        document.getElementById('modal-input-filename').value = file;
      });
    });

    // 15. Viewer Tabs (Text vs Hex)
    document.querySelectorAll('.viewer-tabs .tab-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        document.querySelectorAll('.viewer-tabs .tab-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');

        const view = btn.getAttribute('data-view');
        if (view === 'text') {
          document.getElementById('view-text-pane').classList.add('active');
          document.getElementById('view-hex-pane').classList.remove('active');
        } else {
          document.getElementById('view-text-pane').classList.remove('active');
          document.getElementById('view-hex-pane').classList.add('active');
        }
      });
    });

    // 16. Alert close button
    if (alertClose) {
      alertClose.addEventListener('click', () => {
        alertBanner.classList.add('hidden');
      });
    }
  }

  async function executeApiCall(endpoint, body) {
    try {
      const res = await fetch(endpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      });
      const data = await res.json();

      if (data.success) {
        showToast(data.message || 'Operation completed successfully', 'success');
        hideAlert();
        if (data.state) updateUI(data.state);
        await fetchLogs();
        return true;
      } else {
        const errMsg = data.error || 'System operation failed';
        showToast(errMsg, 'error');
        showAlert(errMsg);
        if (data.state) updateUI(data.state);
        await fetchLogs();
        return false;
      }
    } catch (err) {
      const errMsg = `Network/Server Error: ${err.message}`;
      showToast(errMsg, 'error');
      showAlert(errMsg);
      return false;
    }
  }

  // ==========================================================================
  // Navigation & Modals
  // ==========================================================================

  function initNavigation() {
    const navItems = document.querySelectorAll('.sidebar-nav .nav-item');
    const tabPanes = document.querySelectorAll('.tab-pane');

    navItems.forEach(item => {
      item.addEventListener('click', e => {
        e.preventDefault();
        const tabId = item.getAttribute('data-tab');

        navItems.forEach(n => n.classList.remove('active'));
        item.classList.add('active');

        tabPanes.forEach(pane => {
          if (pane.id === `tab-${tabId}`) {
            pane.classList.add('active');
          } else {
            pane.classList.remove('active');
          }
        });

        // Close sidebar on mobile
        const sidebar = document.querySelector('.sidebar');
        if (sidebar.classList.contains('open')) {
          sidebar.classList.remove('open');
        }
      });
    });

    const sidebarToggle = document.getElementById('sidebar-toggle');
    if (sidebarToggle) {
      sidebarToggle.addEventListener('click', () => {
        document.querySelector('.sidebar').classList.toggle('open');
      });
    }
  }

  function initModals() {
    document.querySelectorAll('.modal-close-btn, .modal-footer .btn-secondary').forEach(btn => {
      btn.addEventListener('click', () => {
        const modalId = btn.getAttribute('data-modal');
        if (modalId) hideModal(modalId);
      });
    });

    document.querySelectorAll('.modal-backdrop').forEach(modal => {
      modal.addEventListener('click', e => {
        if (e.target === modal) {
          modal.classList.add('hidden');
        }
      });
    });
  }

  function showModal(id) {
    const el = document.getElementById(id);
    if (el) el.classList.remove('hidden');
  }

  function hideModal(id) {
    const el = document.getElementById(id);
    if (el) el.classList.add('hidden');
  }

  function showAlert(msg) {
    alertMessage.textContent = msg;
    alertBanner.classList.remove('hidden');
  }

  function hideAlert() {
    alertBanner.classList.add('hidden');
  }

  function showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;

    let icon = '<i class="fa-solid fa-circle-info"></i>';
    if (type === 'success') icon = '<i class="fa-solid fa-circle-check"></i>';
    else if (type === 'error') icon = '<i class="fa-solid fa-triangle-exclamation"></i>';

    toast.innerHTML = `${icon} <span>${escapeHtml(message)}</span>`;
    toastContainer.appendChild(toast);

    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transition = 'opacity 0.3s ease';
      setTimeout(() => toast.remove(), 300);
    }, 4000);
  }

  // ==========================================================================
  // Utilities
  // ==========================================================================

  function formatBytes(bytes) {
    if (bytes === 0 || bytes === undefined || bytes === null) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
  }

  function escapeHtml(str) {
    if (typeof str !== 'string') return str;
    return str.replace(/&/g, '&amp;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;')
              .replace(/"/g, '&quot;')
              .replace(/'/g, '&#039;');
  }
});
