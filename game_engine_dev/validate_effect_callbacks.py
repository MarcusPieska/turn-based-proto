#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_effects_file(filename):
    effects = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 2:
                for i in range(1, len(parts)):
                    effect_pair = parts[i].strip()
                    if effect_pair:
                        if '(' in effect_pair:
                            paren_idx = effect_pair.find('(')
                            effect_name = effect_pair[:paren_idx].strip()
                            if effect_name:
                                effects.add(effect_name)
                        else:
                            equal_split = effect_pair.split('=')
                            if len(equal_split) >= 1:
                                effect_name = equal_split[0].strip()
                                if effect_name:
                                    effects.add(effect_name)
    return effects

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    effects = parse_effects_file("game_config.effects")
    print("Effect callbacks found in game_config.effects:")
    for effect in sorted(effects):
        print(effect)
    print("\nTotal unique effects: " + str(len(effects)))
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
