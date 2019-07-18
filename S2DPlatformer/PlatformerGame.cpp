#include "PlatformerGame.h"

#include <sstream>

// 
// TODO: In editor mode make a "shadow" of a block come up
// TODO: Make a console so that I can load/save custom maps at run time.
// TODO: Stop spawn points + end points from spawning if their is already one on the map/ replace them
// TODO: Make it so gems cannot be placed on the same blocks
// TODO: Make it so you can remove enemies
//

int PlatformerGame::TotalTime = 0;
const int PlatformerGame::WarningTime = 10;
const int PlatformerGame::NumberOfLevels = 3;

PlatformerGame::PlatformerGame(int argc, char* argv[]) : Game(argc, argv), _levelIndex(-1), _level(nullptr)
{
	Audio::Initialise(); //Loads slow - so do it first
	Graphics::Initialise(argc, argv, this, 800, 480, false, 25, 25, "Platformer", 60);
	Input::Initialise(); //Must be initialized after Graphics Initialization
	Graphics::StartGameLoop();
}

PlatformerGame::~PlatformerGame(void)
{
}

//Loads all the content required for the game
void PlatformerGame::LoadContent()
{
	_winOverlay = new Texture2D();
	_winOverlay->Load("Content/Overlays/you_win.png", false);
    _loseOverlay = new Texture2D();
	_loseOverlay->Load("Content/Overlays/you_lose.png", false);
    _diedOverlay = new Texture2D();
	_diedOverlay->Load("Content/Overlays/you_died.png", false);

	_backgroundMusic = new SoundEffect();
	_backgroundMusic->Load("Content/Sounds/Music.wav");
	_backgroundMusic->SetLooping(true);
	Audio::Play(_backgroundMusic);

    LoadNextLevel();
}

//Called every frame to update the game
void PlatformerGame::Update(int elapsedTime)
{
	TotalTime += elapsedTime;

	// Handle polling for our input and handling high-level input
	HandleInput(TotalTime);

	if (!_GamePaused) {

		// update our level, passing down the GameTime along with all of our input states
		_level->Update(elapsedTime);

	}
}

//called every frame to draw the game
void PlatformerGame::Draw(int elapsedTime)
{
	if (!_GamePaused) {
		SpriteBatch::BeginDraw();

		_level->Draw(elapsedTime);

		DrawHud();

		SpriteBatch::EndDraw();
	}
}


std::string listArray[] = { "X", "G", "-", "A", "~", ":", "1", "#" };

//Deals with all the input handling in the game
void PlatformerGame::HandleInput(int elapsedTime)
{
	// get all of our input states
    _keyboardState = Input::Keyboard::GetState();
	_mouseState = Input::Mouse::GetState();	
	if (_level->isLevelEditing())
	{
		if (_mouseState->LeftButton == Input::ButtonState::PRESSED)
		{
			Vector2 vec = _level->screenSpaceToTiles(_mouseState->X, _mouseState->Y);
			vector<vector<Tile*>>* curTiles = (_level->getTiles());
			Texture2D* texture = curTiles->at(vec.X).at(vec.Y)->Texture;
			bool canPrecede = true;


			if (texture == nullptr)
			{
				vector<Gem*> curGems = (_level->getGems());

				for (int i = 0; i < curGems.size(); i++) {

					Gem* curGem = curGems.at(i);

					if (curGem->basePos.X == vec.X && curGem->basePos.Y == vec.Y)
					{
						canPrecede = false;
					}

				}

				if (canPrecede)
				{

					char spawn = '-';

					if (listArray[_level->getLevelEditingID()] != "") {

						string str = listArray[_level->getLevelEditingID()];
						spawn = str[0];

					}

					(*curTiles)[(int)vec.X][(int)vec.Y] = _level->LoadTile(spawn, (int)vec.X, (int)vec.Y);

				}
			}

		}else if (_mouseState->RightButton == Input::ButtonState::PRESSED)
		{
			Vector2 vec = _level->screenSpaceToTiles(_mouseState->X, _mouseState->Y);
			vector<vector<Tile*>>* curTiles = (_level->getTiles());
			Texture2D* texture = curTiles->at(vec.X).at(vec.Y)->Texture;

			if (texture != nullptr)
			{
				delete (*curTiles)[(int)vec.X][(int)vec.Y];
				(*curTiles)[(int)vec.X][(int)vec.Y] = _level->LoadTile((char)'.', (int)vec.X, (int)vec.Y);
			}
			else {

				vector<Gem*> curGems = (_level->getGems());

				for (int i = 0; i < curGems.size(); i++) {

					Gem* curGem = curGems.at(i);

					if (curGem->basePos.X == vec.X && curGem->basePos.Y == vec.Y)
					{
						curGems.erase(curGems.begin() + i--);
						_level->SetGems(curGems);
						break;
					}

				}

			}

		}
	}

    // Exit the game when back is pressed.
	if (_keyboardState->IsKeyDown(Input::Keys::ESCAPE) && (lastPause < elapsedTime))
	{
		//Audio::Destroy();
		//Input::Destroy();
		//Graphics::Destroy();
		lastPause = elapsedTime + 500;
		_GamePaused = !_GamePaused;
	}

	if (_keyboardState->IsKeyDown(Input::Keys::F8)) {
		ReloadCurrentLevel();
		_level->ToggleLevelEditor();
	}

	std::vector< Input::Keys > curKeys = _keyboardState->GetPressedKeys();

	for (int i = 0; i < curKeys.size(); i++) 
	{
		if ((int)curKeys[i] >= (int)Input::Keys::KEY0 && ((int)curKeys[i] <= (int)Input::Keys::KEY9))
		{
			_level->UpdateSpawningID((int)curKeys[i] - (int)Input::Keys::KEY0);
		}
	}

	bool continuePressed = _keyboardState->IsKeyDown(Input::Keys::SPACE) || _keyboardState->IsKeyDown(Input::Keys::W);

    // Perform the appropriate action to advance the game and
    // to get the player back to playing.
    if (!_wasContinuePressed && continuePressed)
    {
        if (!_level->GetPlayer()->IsAlive())
        {
            _level->StartNewLife();
        }
        else if (_level->GetTimeRemaining() <= 0)
        {
			if (_level->ReachedExit()) {
				_level->SaveScore();
				LoadNextLevel();
			}
            else
                ReloadCurrentLevel();
        }
    }

    _wasContinuePressed = continuePressed;
}

void PlatformerGame::LoadNextLevel()
{
	// move to the next level
    _levelIndex = (_levelIndex + 1) % NumberOfLevels;

    // Unloads the content for the current level before loading the next one.
    if (_level != nullptr)
        delete _level;

    // Load the level.
    _level = new Level(_levelIndex);

	_oldScore = 0;
}

void PlatformerGame::ReloadCurrentLevel()
{
	--_levelIndex;
    LoadNextLevel();
}

//Draws the Time and Score (A little messy!)
void PlatformerGame::DrawHud()
{
	Rect titleSafeArea(0.0f, 0.0f, Graphics::GetViewportWidth(), Graphics::GetViewportHeight());
    Vector2 hudLocation(titleSafeArea.X + 10, titleSafeArea.Y + 20);
    Vector2 center(titleSafeArea.X + titleSafeArea.Width / 2.0f,
                                    titleSafeArea.Y + titleSafeArea.Height / 2.0f);

    // Draw time remaining. Uses modulo division to cause blinking when the
    // player is running out of time.
	stringstream timeString;
	timeString << "TIME: " << floor(_level->GetTimeRemaining() / 1000.0f);
    Color timeColor;
    if (_level->GetTimeRemaining() > WarningTime ||
        _level->ReachedExit() ||
        (int)(_level->GetTimeRemaining() / 1000.0f) % 2 == 0)
    {
        timeColor = Color(255, 255, 0);
    }
    else
    {
        timeColor = *Color::Red;
    }
	SpriteBatch::DrawString(timeString.str().c_str(), &(hudLocation + Vector2(1.0f, 1.0f)), Color::Black);
	SpriteBatch::DrawString(timeString.str().c_str(), &hudLocation, &timeColor);

    //Draw score
    float timeHeight = 20;
	stringstream scoreString;
	Color yellow(255, 255, 0);
	Color green(0, 255, 0);

	bool _isHighScore = _oldScore > (_level->GetHighScore() == -842150451 ? 0 : _level->GetHighScore());

	_oldScore = (int)(_oldScore * (1 - 0.1f) + _level->GetScore() * 0.1f);
	scoreString << "SCORE: " << _oldScore;
	if (_isHighScore) {
		scoreString << " (Highscore)";
	}

	SpriteBatch::DrawString(scoreString.str().c_str(), &(hudLocation + Vector2(1.0f, (timeHeight * 1.2f) + 1.0f)), Color::Black);
	SpriteBatch::DrawString(scoreString.str().c_str(), &(hudLocation + Vector2(0.0f, timeHeight * 1.2f)), _isHighScore ? &green : &yellow);
           
	float highscoreHeight = 20.0f * 2;
	stringstream highscoreString;

	highscoreString << "HIGHSCORE: " << (_level->GetHighScore() == -842150451 ? 0 : _level->GetHighScore());
	SpriteBatch::DrawString(highscoreString.str().c_str(), &(hudLocation + Vector2(1.0f, (highscoreHeight * 1.2f) + 1.0f)), Color::Black);
	SpriteBatch::DrawString(highscoreString.str().c_str(), &(hudLocation + Vector2(0.0f, highscoreHeight * 1.2f)), &yellow);

	if (_level->isLevelEditing()) {

		float isLevelEditings = 20.0f * 3;
		stringstream levelEditingString;

		levelEditingString << "Level editing! ID: " << _level->getLevelEditingID();
		SpriteBatch::DrawString(levelEditingString.str().c_str(), &(hudLocation + Vector2(1.0f, (isLevelEditings * 1.2f) + 1.0f)), Color::Black);
		SpriteBatch::DrawString(levelEditingString.str().c_str(), &(hudLocation + Vector2(0.0f, isLevelEditings * 1.2f)), Color::Red);

	}

    //Determine the status overlay message to show.
    Texture2D* status = nullptr;
    if (_level->GetTimeRemaining() == 0)
    {
        if (_level->ReachedExit())
        {
            status = _winOverlay;
        }
        else
        {
            status = _loseOverlay;
        }
    }
    else if (!_level->GetPlayer()->IsAlive())
    {
        status = _diedOverlay;
    }

    if (status != nullptr)
    {
        // Draw status message.
        Vector2 statusSize((float)status->GetWidth(), (float)status->GetHeight());
		SpriteBatch::Draw(status, &(center - statusSize / 2));
    }
}