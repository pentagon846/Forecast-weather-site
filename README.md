# 🌦 Flask Weather Forecast Application
This is a simple weather forecast web application built using the Flask framework. It provides a 4-day weather forecast for a specific location based on data from an external weather API.

# ✨ Features
4-day weather forecast for a selected location 
Simple and intuitive user interface 
Real-time data fetched from an external weather API
Mobile-friendly and responsive design
# 🚀 Getting Started
Prerequisites
Before running the project, make sure you have installed the following:

Python 3.8+
Flask
Requests (for API calls)
A valid API key from a weather service provider (e.g., OpenWeatherMap, WeatherAPI, etc.)
Installation
Clone the repository:

git clone https://github.com/your-username/weather-flask-app.git
cd weather-flask-app

Create a virtual environment and activate it:

python -m venv venv
source venv/bin/activate  # For Linux/macOS
or
venv\Scripts\activate  # For Windows

### Install dependencies:

pip install -r requirements.txt

Set up environment variables:

In the root directory of the project, create a .env file and add your API key:

API_KEY=your_weather_api_key

Run the application:

flask run


# 🛠 Configuration
The API configuration is stored in the .env file. Replace your_weather_api_key with your actual API key from the weather provider of your choice.

# 🌎 Usage
Once the application is running, simply enter the name of a city in the input field, and it will fetch the weather forecast for the next 4 days.

You will be able to see the following information for each day:

Temperature (Celsius/Fahrenheit)
Weather description (sunny, cloudy, rainy, etc.)
Wind speed
Humidity

# 🤝 Contributing
Feel free to open issues or submit pull requests if you would like to improve the application. All contributions are welcome!

# 📜 License
This project is licensed under the MIT License. See the LICENSE file for more details.


