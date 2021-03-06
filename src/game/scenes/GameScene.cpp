#include "GameScene.h"
#include "../Game.h"
#include "../audio/MusicManager.h"
#include "MainMenuScene.h"
#include <random>
#include <iostream>

Game::Scenes::GameScene::GameScene(Game& game, size_t sizeX, size_t sizeY, size_t minesNumber)
: Scene(game), m_field(sizeX, sizeY) {
    this->m_minesNumber = minesNumber;
    generateMines(minesNumber);
    calculateNearbyMinesCount();
    Audio::MusicManager::getInstance().playMusic("resources/music/game-theme.ogg");
}

void Game::Scenes::GameScene::onEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        float rectangleSizeX = static_cast<float>(m_game.getWindow().getSize().x) / static_cast<float>(m_field.getSizeX());
        float rectangleSizeY = static_cast<float>(m_game.getWindow().getSize().y) / static_cast<float>(m_field.getSizeY());
        size_t cellX = event.mouseButton.x / rectangleSizeX;
        size_t cellY = event.mouseButton.y / rectangleSizeY;
        Cell& cell = m_field.get(cellX, cellY);

        if (event.mouseButton.button == sf::Mouse::Left) {
            if (!cell.isChecked) {
                checkCell(cellX, cellY);
            }
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            if (!cell.isChecked) {
                cell.isMarked = !cell.isMarked;
            }
        }

        if (getFieldStatus() == Field::FieldStatus::Lose) {
            m_game.changeScene(std::make_unique<MainMenuScene>(m_game));
        } else if (getFieldStatus() == Field::FieldStatus::Win) {
            m_game.changeScene(std::make_unique<MainMenuScene>(m_game));
        }
    }
}

void Game::Scenes::GameScene::onUpdate(const sf::Time& elapsedTime) {
    float rectangleSizeX = static_cast<float>(m_game.getWindow().getSize().x) / static_cast<float>(m_field.getSizeX());
    float rectangleSizeY = static_cast<float>(m_game.getWindow().getSize().y) / static_cast<float>(m_field.getSizeY());
    sf::RectangleShape shape(sf::Vector2f(rectangleSizeX, rectangleSizeY));
    sf::Font font;
    sf::Text text;

    font.loadFromFile("resources/fonts/arial.ttf");
    shape.setOutlineThickness(1.0);
    shape.setOutlineColor(sf::Color::Black);
    text.setFont(font);
    text.setCharacterSize(std::min(rectangleSizeX, rectangleSizeY) / 2);
    text.setFillColor(sf::Color::Black);

    for (size_t y = 0; y < m_field.getSizeY(); ++y) {
        for (size_t x = 0; x < m_field.getSizeX(); ++x) {
            Cell& cell = m_field.get(x, y);

            shape.setPosition(x * rectangleSizeX, y * rectangleSizeY);
            text.setPosition(x * rectangleSizeX,
                             y * rectangleSizeY);

            if (cell.isChecked) {
                if (cell.isMine) {
                    shape.setFillColor(sf::Color::Red);
                } else {
                    shape.setFillColor(sf::Color::White);
                }

                text.setString(std::to_string(cell.nearbyMinesNumber));
            } else if (cell.isMarked) {
                shape.setFillColor(sf::Color::Yellow);
            } else {
                shape.setFillColor(sf::Color(100, 100, 100));
            }

            m_game.getWindow().draw(shape);
            m_game.getWindow().draw(text);
            text.setString("");
        }
    }
}

void Game::Scenes::GameScene::generateMines(size_t minesNumber) {
    std::random_device rd;
    std::mt19937 mersenne(rd());
    size_t sizeX = m_field.getSizeX();
    size_t sizeY = m_field.getSizeX();

    if (minesNumber > sizeX * sizeY) {
        throw std::invalid_argument("Mines count greater than cells count.");
    }

    for (size_t i = 0; i < minesNumber; ++i) {
        size_t generatedX = mersenne() % sizeX;
        size_t generatedY = mersenne() % sizeY;

        while (m_field.get(generatedX, generatedY).isMine) {
            generatedX = mersenne() % sizeX;
            generatedY = mersenne() % sizeY;
        }

        Cell& cell = m_field.get(generatedX, generatedY);
        cell.isMine = true;
    }
}

void Game::Scenes::GameScene::calculateNearbyMinesCount() {
    for (size_t y = 0; y < m_field.getSizeY(); ++y) {
        for (size_t x = 0; x < m_field.getSizeX(); ++x) {
            size_t minesCount = 0;
            std::vector<std::pair<size_t, size_t>> shifts = {{-1, -1}, {-1, 0}, {-1, 1},
                                                             {0, -1}, {0, 1},
                                                             {1, -1}, {1, 0}, {1, 1}};

            for (const auto& shift : shifts) {
                try {
                    if (m_field.get(x + shift.first, y + shift.second).isMine) {
                        ++minesCount;
                    }
                } catch (const std::invalid_argument&) {
                    // do nothing
                }
            }

            m_field.get(x, y).nearbyMinesNumber = minesCount;
        }
    }
}

void Game::Scenes::GameScene::checkCell(size_t x, size_t y) {
    Cell& cell = m_field.get(x, y);

    if (cell.isChecked) {
        return;
    }

    cell.isChecked = true;

    if (cell.nearbyMinesNumber == 0) {
        std::vector<std::pair<size_t, size_t>> shifts = {{-1, -1}, {-1, 0}, {-1, 1},
                                                         {0, -1}, {0, 1},
                                                         {1, -1}, {1, 0}, {1, 1}};

        for (const auto& shift : shifts) {
            try {
                checkCell(x + shift.first, y + shift.second);
            } catch (const std::invalid_argument&) {
                // do nothing
            }
        }
    }
}

Game::Field::FieldStatus Game::Scenes::GameScene::getFieldStatus() {
    size_t markedCells = 0;
    size_t markedMines = 0;
    size_t uncheckedCells = 0;

    for (size_t y = 0; y < m_field.getSizeY(); ++y) {
        for (size_t x = 0; x < m_field.getSizeX(); ++x) {
            try {
                Cell& cell = m_field.get(x, y);

                if (cell.isChecked) {
                    if (cell.isMine) {
                        return Field::FieldStatus::Lose;
                    }
                } else {
                    if (cell.isMarked) {
                        if (cell.isMine) {
                            ++markedMines;
                        } else {
                            ++markedCells;
                        }
                    } else {
                        ++uncheckedCells;
                    }
                }
            } catch (const std::invalid_argument&) {
                // do nothing
            }
        }
    }

    if (markedMines == m_minesNumber && markedCells == 0 && uncheckedCells == 0) {
        return Field::FieldStatus::Win;
    }

    return Field::FieldStatus::Unknown;
}
