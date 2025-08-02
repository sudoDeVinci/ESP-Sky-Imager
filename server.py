from flask import (
    Flask,
    Blueprint,
    Response,
    request,
    jsonify
)

apiRouter = Blueprint('api', __name__, url_prefix='/api')
"""
API endpoints for handling status and environmental reading data.
"""

def display_data() -> tuple[Response, int]:
    """
    Display the data received from the API.
    """
    try:
        headers = request.headers
        data = request.get_json()
        
        if not data:
            return jsonify({"error": "No data provided"}), 400
        
        print("Received headers:", headers)
        print("Received data:", data)
        
        return jsonify({"message": "Data received successfully"}), 200
    except Exception as e:
        return jsonify({"error": "Internal server error"}), 500

@apiRouter.route('/status', methods=['POST', 'PUT'])
def status():
    """
    Endpoint to handle status updates.
    """
    return display_data()

@apiRouter.route('/reading', methods=['POST', 'PUT'])
def reading():
    """
    Endpoint to handle environmental reading updates.
    """
    return display_data()

@apiRouter.route('/qnh', methods=['GET'])
def qnh():
    """
    Endpoint to handle QNH updates.
    """
    return jsonify({"qnh": 1000}), 200


@apiRouter.route('/version', methods=['GET'])
def version():
    """
    Endpoint to handle firmware version updates.
    """
    return jsonify({"version": "1.0.0"}), 200


@apiRouter.route('/check', methods=['GET'])
def index():
    print("API check endpoint accessed")
    return jsonify ({"message": "Welcome to the API!"}), 200





if __name__ == '__main__':
    app = Flask(__name__)
    app.register_blueprint(apiRouter)

    app.run(host='0.0.0.0', port=5000, debug=True)






