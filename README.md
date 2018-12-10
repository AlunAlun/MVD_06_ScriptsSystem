### La Salle - Universitat Ramon Llull : Master in Advanced Videogame Development
## Engine Programming - 07-ScriptsSystem

### TODO 

- create a new class called MovePlatformScript which is derived from Script (owner, teal floor entity)
- in update function of script, translate platform up
- add boolean to start/stop translate when key is pressed 
 - use (input_->GetKey(GLFW_KEY_xxxx) where xxxx is the key code


- create new pointer instance of MovePlaformScript
- register it with scripts_system


- create a SwitchScript derived Script (owner green floor entity)
- script gets collider component attached to owner entity
- script also has pointer to MovePlatformScript
- if collider is colliding, set boolean of MovePlatformScript to make it move
- don't forget to instantiate and register SwitchScript here in Game::init

- create another script with custom behviour, for example a platform that moves up and down automatically

- Study the FPS movement code in ControlSystem to see how it works
