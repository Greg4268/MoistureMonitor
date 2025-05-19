 // Auto-refresh functionality
        let autoRefresh = true;
        let countdown = 5;
        let countdownInterval;

        function updateCountdown() {
            document.getElementById('countdown').textContent = countdown;
            if (countdown <= 0 && autoRefresh) {
                refreshData();
                countdown = 5;
            } else {
                countdown--;
            }
        }

        function startCountdown() {
            countdownInterval = setInterval(updateCountdown, 1000);
        }

        function refreshData() {
            window.location.reload();
        }

        function toggleAutoRefresh() {
            autoRefresh = !autoRefresh;
            const btn = document.getElementById('autoRefreshBtn');
            
            if (autoRefresh) {
                btn.textContent = 'Pause Auto-Refresh';
                countdown = 5;
                startCountdown();
            } else {
                btn.textContent = 'Resume Auto-Refresh';
                clearInterval(countdownInterval);
                document.getElementById('countdown').textContent = '∞';
            }
        }

        function exportData() {
            window.open('/export-data', '_blank');
        }

        // Initialize chart if data is available
        document.addEventListener('DOMContentLoaded', function() {
            const chartDataElement = document.getElementById('chartData');
            const chartInfo = JSON.parse(chartDataElement.textContent);
            
            if (chartInfo.hasHistory && chartInfo.chartLabels.length > 1) {
                const ctx = document.getElementById('humidityChart').getContext('2d');
                
                const humidityData = {
                    labels: chartInfo.chartLabels,
                    datasets: [{
                        label: 'Humidity (%)',
                        data: chartInfo.chartData,
                        borderColor: 'rgb(75, 192, 192)',
                        backgroundColor: 'rgba(75, 192, 192, 0.1)',
                        tension: 0.1,
                        fill: true
                    }]
                };

                const config = {
                    type: 'line',
                    data: humidityData,
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        scales: {
                            y: {
                                beginAtZero: false,
                                min: 0,
                                max: 100,
                                title: {
                                    display: true,
                                    text: 'Humidity (%)'
                                }
                            },
                            x: {
                                title: {
                                    display: true,
                                    text: 'Time'
                                },
                                ticks: {
                                    maxTicksLimit: 10
                                }
                            }
                        },
                        plugins: {
                            legend: {
                                display: true
                            }
                        }
                    }
                };

                const humidityChart = new Chart(ctx, config);
            }
        });

        // Start auto-refresh
        startCountdown();