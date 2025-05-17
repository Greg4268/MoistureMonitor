from flask import Flask, request, jsonify, render_template, redirect, url_for, send_from_directory
from flask_httpauth import HTTPBasicAuth
from werkzeug.security import generate_password_hash, check_password_hash
from werkzeug.utils import secure_filename
import os 
import time 
import logging 
from datetime import datetime 

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

# Create flask app 
app = Flask(__name__, template_folder='../../templates')
auth = HTTPBasicAuth()

users = {
    os.environ.get('AUTH_USERNAME'): generate_password_hash(os.environ.get('AUTH_PASSWORD'))
}

if None in users or not users:
    raise ValueError("AUTH_USERNAME and AUTH_PASSWORD environment variables must be set")

# Store the current alert status
current_alert = {
    "status": "good",
    "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
}

@auth.verify_password
def verify_password(username, password):
    if username in users and check_password_hash(users.get(username), password):
        return username 
    return None

@app.route('/ping')
def ping():
    return "Server is running"

# Home 
@app.route('/')
@auth.login_required
def index():
    logger.debug("Accessing index page")
    try:
        logger.debug(f"Current alert status: {current_alert['status']}")
        return render_template('index.html', alert=current_alert)
    except Exception as e: 
        logger.error(f"Error in index route: {str(e)}")
        return f"Error: {str(e)}", 500

# Alert update endpoint
@app.route('/update-alert', methods=['POST'])
def update_alert():
    logger.debug("Alert update endpoint hit")
    
    try:
        # Get data from request
        data = request.get_json()
        
        if not data or 'status' not in data:
            logger.error("No alert status in request")
            return jsonify({'error': 'Missing alert status'}), 400
        
        # Validate the alert status
        status = data['status']
        valid_statuses = ["good", "warning(approaching bad)", "warning(bad)"]
        
        if status not in valid_statuses:
            logger.error(f"Invalid alert status: {status}")
            return jsonify({'error': 'Invalid alert status'}), 400
        
        # Update the current alert
        current_alert["status"] = status
        current_alert["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        logger.info(f"Alert updated to: {status}")
        
        return jsonify({
            'message': 'Alert updated successfully',
            'status': status,
            'timestamp': current_alert["timestamp"]
        }), 200
        
    except Exception as e:
        logger.error(f"Error updating alert: {str(e)}")
        return jsonify({'error': str(e)}), 500

# API endpoint to get current alert status (useful for clients that need to check)
@app.route('/get-alert', methods=['GET'])
def get_alert():
    return jsonify(current_alert), 200

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    logger.info(f"Starting server on port {port}")
    app.run(host="0.0.0.0", port=port)