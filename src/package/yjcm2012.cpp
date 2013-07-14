#include "yjcm2012.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "maneuvering.h"

class Zhenlie: public TriggerSkill{
public:
    Zhenlie():TriggerSkill("zhenlie"){
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.from == effect.to || (!effect.card->isKindOf("Slash") && !effect.card->isNDTrick()))
            return false;

        if(player->askForSkillInvoke(objectName(), data)){
            room->playSkillEffect(objectName());
            room->loseHp(player);
            if(!effect.from->isNude()){
                int card_id = room->askForCardChosen(player, effect.from, "he", objectName());
                room->throwCard(card_id, effect.from, player);
            }
            return true;
        }
        return false;
    }
};

MijiCard::MijiCard(){
    will_throw = false;
}

bool MijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void MijiCard::use(Room *room, ServerPlayer *, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *beggar = targets.first();

    room->moveCardTo(this, beggar, Player::Hand, false);
    room->setEmotion(beggar, "draw-card");
}

class MijiViewAsSkill: public ViewAsSkill{
public:
    MijiViewAsSkill():ViewAsSkill("miji"){
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(to_select->isEquipped())
            return false;

        return selected.length() < Self->getMark("miji");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != Self->getMark("miji"))
            return NULL;

        MijiCard *card = new MijiCard;
        card->addSubcards(cards);
        return card;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@miji!";
    }
};

class Miji: public PhaseChangeSkill{
public:
    Miji():PhaseChangeSkill("miji"){
        view_as_skill = new MijiViewAsSkill;
    }

    virtual QString getDefaultChoice(ServerPlayer *player) const{
        return QString::number(player->getLostHp());
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const{
        if(wangyi->getPhase() == Player::Finish && wangyi->isWounded()){
            Room *room = wangyi->getRoom();
            QStringList choices;
            for(int i = wangyi->getLostHp(); i > 0; i--)
                choices << QString::number(i);
            choices.append("cancel");
            QString choice = room->askForChoice(wangyi, objectName(), choices.join("+"));
            if(choice == "cancel")
                return false;
            int x = choice.toInt();
            wangyi->drawCards(x);
            room->setPlayerMark(wangyi, "miji", x);
            bool used = room->askForUseCard(wangyi, "@@miji!", "@miji:::" + choice);

            if(!used){
                QList<int> to_give = wangyi->handCards().mid(0, x);
                MijiCard *miji_card = new MijiCard;
                foreach(int card_id, to_give)
                    miji_card->addSubcard(card_id);
                QList<ServerPlayer *> targets = room->getOtherPlayers(wangyi);
                miji_card->use(room, wangyi, targets);
                delete miji_card;
            }
        }
        return false;
    }
};

QiceCard::QiceCard(){
    will_throw = false;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card);
}

bool QiceCard::targetFixed() const{
    if(ClientInstance->getStatus() == Client::Responsing)
        return true;

    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetsFeasible(targets, Self);
}

Card::Suit QiceCard::getSuit(QList<int> cardid_list) const{
    QSet<QString> suit_set;
    QSet<Card::Color> color_set;
    foreach(int id, cardid_list){
        const Card *cd = Sanguosha->getCard(id);
        suit_set << cd->getSuitString();
        color_set << cd->getColor();
    }
    if(color_set.size() == 2)
        return Card::NoSuit;
    else{
        if(suit_set.size() == 1)
            return Sanguosha->getCard(cardid_list.first())->getSuit();
        else{
            if(Sanguosha->getCard(cardid_list.first())->isBlack())
                return Card::Spade;
            else
                return Card::Heart;
        }
    }
}

int QiceCard::getNumber(QList<int> cardid_list) const{
    if(cardid_list.length() == 1)
        return Sanguosha->getCard(cardid_list.first())->getNumber();
    else
        return 0;
}

const Card *QiceCard::validate(const CardUseStruct *card_use) const{
    Room *room = card_use->from->getRoom();
    card_use->from->setFlags("QiceUsed");
    room->playSkillEffect("qice");
    Card *use_card = Sanguosha->cloneCard(user_string, getSuit(this->getSubcards()), getNumber(this->getSubcards()));
    use_card->setSkillName("qice");
    foreach(int id, this->getSubcards())
        use_card->addSubcard(id);
    return use_card;
}

const Card *QiceCard::validateInResposing(ServerPlayer *xunyou, bool *continuable) const{
    *continuable = true;

    Room *room = xunyou->getRoom();
    room->playSkillEffect("qice");
    xunyou->setFlags("QiceUsed");

    Card *use_card = Sanguosha->cloneCard(user_string, getSuit(this->getSubcards()), getNumber(this->getSubcards()));
    use_card->setSkillName("qice");
    foreach(int id, this->getSubcards())
        use_card->addSubcard(id);

    return use_card;
}

class Qice: public ViewAsSkill{
public:
    Qice():ViewAsSkill("qice"){
    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::getInstance("qice", false);
    }

    virtual bool viewFilter(const QList<CardItem *> &, const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() < Self->getHandcardNum())
            return NULL;

        if(ClientInstance->getStatus() == Client::Responsing){
            QiceCard *card = new QiceCard;
            card->setUserString("nullification");
            card->addSubcards(cards);
            return card;
        }

        CardStar c = Self->tag.value("qice").value<CardStar>();
        if(c){
            QiceCard *card = new QiceCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        }else
            return NULL;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        foreach(const Card *card, player->getHandcards()){
            if(card->objectName() == "nullification")
                return true;
        }
        return !player->hasFlag("QiceUsed") && !player->isKongcheng() && player->getPhase() == Player::Play;
    }
protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        if(player->isKongcheng())
            return false;
        else
            return !player->hasUsed("QiceCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" &&
                !player->hasFlag("QiceUsed") &&
                !player->isKongcheng() &&
                player->getPhase() == Player::Play ;
    }
};

class Zhiyu: public MasochismSkill{
public:
    Zhiyu():MasochismSkill("zhiyu"){

    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        QVariant source = QVariant::fromValue(from);
        if(from && from->isAlive() && target->askForSkillInvoke(objectName(), source)){
            target->drawCards(1);

            Room *room = target->getRoom();
            room->playSkillEffect(objectName());
            room->showAllCards(target, true);

            QList<const Card *> cards = target->getHandcards();
            Card::Color color = cards.first()->getColor();
            bool same_color = true;
            foreach(const Card *card, cards){
                if(card->getColor() != color){
                    same_color = false;
                    break;
                }
            }

            if(same_color && damage.from && !damage.from->isKongcheng())
                room->askForDiscard(damage.from, objectName(), 1);

            room->broadcastInvoke("clearAG");
        }
    }
};

class Jiangchi:public DrawCardsSkill{
public:
    Jiangchi():DrawCardsSkill("jiangchi"){
    }

    virtual int getDrawNum(ServerPlayer *caozhang, int n) const{
        Room *room = caozhang->getRoom();
        QString choice = room->askForChoice(caozhang, objectName(), "jiang+chi+cancel");
        if(choice == "cancel")
            return n;
        LogMessage log;
        log.from = caozhang;
        log.arg = objectName();
        if(choice == "jiang"){
            log.type = "#Jiangchi1";
            room->sendLog(log);
            room->playSkillEffect(objectName(), 1);
            //room->setPlayerCardLock(caozhang, "slash");
            room->setPlayerFlag(caozhang, "jiangchi_forbid");
            return n + 1;
        }else{
            log.type = "#Jiangchi2";
            room->sendLog(log);
            room->playSkillEffect(objectName(), 2);
            room->setPlayerFlag(caozhang, "jiangchi_invoke");
            return n - 1;
        }
    }
};

class JiangchiForbid: public TriggerSkill{
public:
    JiangchiForbid():TriggerSkill("#jiangchi_forbid"){
        events << CardAsk << CardUseAsk;
    }

    virtual bool trigger(TriggerEvent, Room* , ServerPlayer *caozhang, QVariant &data) const{
        QString asked = data.toString();
        if(asked == "slash" && caozhang->hasFlag("jiangchi_forbid")){
            /*room->playSkillEffect(objectName(), qrand() % 2 + 3);
            LogMessage log;
            log.type = "#Jiangchi";
            log.from = caozhang;
            log.arg = asked;
            log.arg2 = objectName();
            room->sendLog(log);*/
            return true;
        }
        return false;
    }
};

class JiangchiSlash: public SlashSkill{
public:
    JiangchiSlash():SlashSkill("#jiangchi_slash"){
        frequency = NotFrequent;
    }

    virtual int getSlashResidue(const Player *t) const{
        if(t->hasSkill("jiangchi")){
            if(t->hasFlag("jiangchi_invoke"))
                return qMax(2 - t->getSlashCount(), 0);
            else if(t->hasFlag("jiangchi_forbid"))
                return -998;
        }
        return 0;
    }

    virtual int getSlashRange(const Player *from, const Player *, const Card *) const{
        if(from->hasSkill("jiangchi") && from->hasFlag("jiangchi_invoke"))
            return 998;
        else
            return 0;
    }
};

class Qianxi: public TriggerSkill{
public:
    Qianxi():TriggerSkill("qianxi"){
        events << PhaseChange << PhaseEnd << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &) const{
        if(event == PhaseChange && player->getPhase() == Player::Start){
            if(player->askForSkillInvoke(objectName())){
                room->playSkillEffect(objectName());
                JudgeStruct judge;
                judge.who = player;
                judge.reason = objectName();

                room->judge(judge);
                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *tmp, room->getOtherPlayers(player))
                    if(player->distanceTo(tmp) == 1)
                        targets << tmp;
                if(!targets.isEmpty()){
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                    //room->setPlayerFlag(target, "qianxi");
                    target->gainMark("@sneak");

                    LogMessage log;
                    log.type = "#QianXi";
                    log.from = target;
                    log.arg = judge.card->getColorString();
                    room->sendLog(log);

                    room->setPlayerCardLock(target, ".|.|.|hand|" + judge.card->getColorString());
                }
            }
        }
        else if(event == Death || (event == PhaseEnd && player->getPhase() == Player::Finish)){
            foreach(ServerPlayer *tmp, room->getAlivePlayers()) {
                if(tmp->hasMark("@sneak")){
                    room->setPlayerMark(tmp, "@sneak", 0);
                    room->setPlayerCardLock(tmp, "--");
                }
            }
        }
        return false;
    }
};

class Dangxian: public PhaseChangeSkill{
public:
    Dangxian():PhaseChangeSkill("dangxian"){
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *liaohua) const{
        Room *room = liaohua->getRoom();
        if(liaohua->getPhase() == Player::RoundStart){
            room->playSkillEffect(objectName());
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = liaohua;
            log.arg = objectName();
            room->sendLog(log);

            QList<Player::Phase> phases = liaohua->getPhases();
            phases.prepend(Player::Play) ;
            liaohua->play(phases);
        }
        return false;
    }
};

class Fuli: public TriggerSkill{
public:
    Fuli():TriggerSkill("fuli"){
        events << Dying;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@laoji") > 0;
    }

    int getKingdoms(Room *room) const{
        QSet<QString> kingdom_set;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            kingdom_set << p->getKingdom();
        }
        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *liaohua, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != liaohua)
            return false;
        if(liaohua->askForSkillInvoke(objectName(), data)){
            room->playLightbox(liaohua, objectName(), "", 1500);

            liaohua->loseMark("@laoji");
            int x = getKingdoms(room);
            RecoverStruct rev;
            rev.recover = x - liaohua->getHp();
            room->recover(liaohua, rev);
            liaohua->turnOver();
        }
        return false;
    }
};

class FuhunViewAsSkill:public ViewAsSkill{
public:
    FuhunViewAsSkill():ViewAsSkill("fuhun"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const QList<CardItem *> &, const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 2){
            const Card *first = cards.first()->getCard();
            Slash *aa = new Slash(first->getSuit(), 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        }else
            return NULL;
    }
};

class Fuhun: public TriggerSkill{
public:
    Fuhun():TriggerSkill("fuhun"){
        events << Damage << PhaseEnd;
        view_as_skill = new FuhunViewAsSkill;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(event == Damage){
            DamageStruct damage = data.value<DamageStruct>();
            if(player->getPhase() != Player::Play)
                return false;
            if(damage.card->getSkillName() == objectName()){
                room->acquireSkill(player, "wusheng");
                room->acquireSkill(player, "paoxiao");
                player->setFlags(objectName());
            }
        }
        else{
            if(player->getPhase() == Player::Finish && player->hasFlag(objectName())){
                room->detachSkillFromPlayer(player, "wusheng");
                room->detachSkillFromPlayer(player, "paoxiao");
                //room->setPlayerFlag(player, "-" + objectName());
            }
        }
        return false;
    }
};

class Zishou:public DrawCardsSkill{
public:
    Zishou():DrawCardsSkill("zishou"){
    }

    virtual int getDrawNum(ServerPlayer *liubiao, int n) const{
        Room *room = liubiao->getRoom();
        if(liubiao->getLostHp() > 0 && room->askForSkillInvoke(liubiao, objectName())){
            room->playSkillEffect(objectName(), liubiao->getLostHp());
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + liubiao->getLostHp();
        }else
            return n;
    }
};

class Zongshi: public MaxCardsSkill{
public:
    Zongshi():MaxCardsSkill("zongshi"){
    }
    virtual int getExtra(const Player *target) const{
        int extra = 0;
        QSet<QString> kingdom_set;
        if(target->parent()){
            foreach(const Player *player, target->parent()->findChildren<const Player *>()){
                if(player->isAlive()){
                    kingdom_set << player->getKingdom();
                }
            }
        }
        extra = kingdom_set.size();
        if(target->hasSkill(objectName()))
            return extra;
        else
            return 0;
    }
};

class Shiyong: public TriggerSkill{
public:
    Shiyong():TriggerSkill("shiyong"){
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        if(!player->isAlive())
            return false;

        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->inherits("Slash") &&
                (damage.card->isRed() || damage.card->hasFlag("drank"))){
            int index = damage.card->hasFlag("drank") ? 2 : damage.from->getGeneral()->nameContains("guanyu") ? 3 : 1;
            room->playSkillEffect(objectName(), index);

            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            room->loseMaxHp(player);
        }
        return false;
    }
};

GongqiCard::GongqiCard(){
}

bool GongqiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    if(!targets.isEmpty())
        return false;
    return true;
}

bool GongqiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    if(Sanguosha->getCard(getSubcards().first())->isKindOf("EquipCard"))
        return targets.length() <= 1;
    else
        return targets.isEmpty();
}

void GongqiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->setPlayerFlag(source, "gongqi_range");
    if(Sanguosha->getCard(getSubcards().first())->isKindOf("EquipCard") && !targets.isEmpty()){
        PlayerStar target = targets.first();
        int card_id = room->askForCardChosen(source, target, "he", skill_name);
        room->throwCard(card_id, target, source);
    }
}

class Gongqi: public OneCardViewAsSkill{
public:
    Gongqi():OneCardViewAsSkill("gongqi"){
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GongqiCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new GongqiCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class GongqiSlash: public SlashSkill{
public:
    GongqiSlash():SlashSkill("#gongqi_slash"){
        frequency = NotFrequent;
    }

    virtual int getSlashRange(const Player *from, const Player *, const Card *) const{
        if(from->hasSkill("gongqi") && from->hasFlag("gongqi_range"))
            return 998;
        else
            return 0;
    }
};

JiefanCard::JiefanCard(){
}

bool JiefanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const{
    return targets.isEmpty();
}

void JiefanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &tar) const{
    source->loseMark("@bother");
    if(!Config.DisableLightbox)
        room->broadcastInvoke("animate", "lightbox:$jiefan");
    else
        room->setEmotion(source, "skill/" + skill_name);
    room->getThread()->delay(1500);
    PlayerStar target = tar.first();
    foreach(ServerPlayer *p, room->getAllPlayers()){
        if(p->inMyAttackRange(target)){
            if(!room->askForCard(p, "Weapon", "@jiefan:" + source->objectName() + ":" + target->objectName(), QVariant(), CardDiscarded))
                target->drawCards(1);
        }
    }
}

class Jiefan:public ZeroCardViewAsSkill{
public:
    Jiefan():ZeroCardViewAsSkill("jiefan"){
        frequency = Limited;
    }

    virtual const Card *viewAs() const{
        return new JiefanCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@bother") >= 1;
    }
};

AnxuCard::AnxuCard(){
    once = true;
    mute = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;
    if(targets.isEmpty())
        return true;
    else if(targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    int index = 1;
    if(from->getGeneral()->nameContains("sunquan") || to->getGeneral()->nameContains("sunquan"))
        index = 2;
    room->playSkillEffect(skill_name, index);
    int id = room->askForCardChosen(from, to, "h", "anxu");
    const Card *cd = Sanguosha->getCard(id);
    room->obtainCard(from, cd);
    room->showCard(from, id);
    if(cd->getSuit() != Card::Spade){
        source->drawCards(1);
    }
}

class Anxu: public ZeroCardViewAsSkill{
public:
    Anxu():ZeroCardViewAsSkill("anxu"){
    }

    virtual const Card *viewAs() const{
        return new AnxuCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("AnxuCard");
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const{
        return false;
    }
};

class Zhuiyi: public TriggerSkill{
public:
    Zhuiyi():TriggerSkill("zhuiyi"){
        events << Death ;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();
        QList<ServerPlayer *> targets = (damage && damage->from) ?
                            room->getOtherPlayers(damage->from) : room->getAlivePlayers();

        if(targets.isEmpty() || !player->askForSkillInvoke(objectName(), data))
            return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
        room->playSkillEffect(objectName(), target->getGeneral()->nameContains("sunquan") ? 2 : 1);

        target->drawCards(3);
        RecoverStruct recover;
        recover.who = target;
        recover.recover = 1;
        room->recover(target, recover, true);
        return false;
    }
};

class LihuoViewAsSkill:public OneCardViewAsSkill{
public:
    LihuoViewAsSkill():OneCardViewAsSkill("lihuo"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->objectName() == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *acard = new FireSlash(card->getSuit(), card->getNumber());
        acard->addSubcard(card_item->getFilteredCard());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Lihuo: public TriggerSkill{
public:
    Lihuo():TriggerSkill("lihuo"){
        events << Damage << CardFinished;
        view_as_skill = new LihuoViewAsSkill;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(event == Damage){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.card && damage.card->inherits("Slash") && damage.card->getSkillName() == objectName()){
                player->tag["Invokelihuo"] = true;
                room->playSkillEffect(objectName(), 2);
            }
        }
        else if(player->tag.value("Invokelihuo", false).toBool()){
            player->tag["Invokelihuo"] = false;
            room->loseHp(player, 1);
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 1;
    }
};

class LihuoSlash: public SlashSkill{
public:
    LihuoSlash():SlashSkill("#lihuo_slash"){
        frequency = NotFrequent;
    }

    virtual int getSlashExtraGoals(const Player *from, const Player *, const Card *slash) const{
        if(from->hasSkill("lihuo") && slash &&
                (slash->inherits("FireSlash") || slash->getSkillName() == "lihuo"))
            return 1;
        else
            return 0;
    }
};

ChunlaoCard::ChunlaoCard(){
    will_throw = false;
    target_fixed = true;
    mute = true;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    foreach(int id, this->subcards)
        source->addToPile("wine", id, true);
    source->playSkillEffect(skill_name, 1);
}

class ChunlaoViewAsSkill:public ViewAsSkill{
public:
    ChunlaoViewAsSkill():ViewAsSkill("chunlao"){
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@chunlao";
    }

    virtual bool viewFilter(const QList<CardItem *> &, const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("Slash");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 0)
            return NULL;

        Card *acard = new ChunlaoCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Chunlao: public TriggerSkill{
public:
    Chunlao():TriggerSkill("chunlao"){
        events << PhaseChange << Dying ;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *, QVariant &data) const{
        ServerPlayer *chengpu = room->findPlayerBySkillName(objectName());
        if(!chengpu)
            return false;
        if(event == PhaseChange){
            if(chengpu->getPhase() == Player::Finish && !chengpu->isKongcheng() &&
                chengpu->getPile("wine").isEmpty())
                room->askForUseCard(chengpu, "@@chunlao", "@chunlao");
        }else if(event == Dying){
            DyingStruct dying = data.value<DyingStruct>();
            while(dying.who->getHp() < 1 && !chengpu->getPile("wine").isEmpty() && chengpu->askForSkillInvoke(objectName(), data)){
                QList<int> cards = chengpu->getPile("wine");
                if(cards.isEmpty())
                    break;
                room->fillAG(cards, chengpu);
                int card_id = room->askForAG(chengpu, cards, true, objectName());
                room->broadcastInvoke("clearAG");
                if(card_id != -1){
                    room->throwCard(card_id);
                    Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
                    analeptic->setSkillName(objectName());
                    CardUseStruct use;
                    use.card = analeptic;
                    use.from = dying.who;
                    use.to << dying.who;
                    room->useCard(use);
                }
            }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *who, const Card *) const{
        if(who->getGeneral()->nameContains("zhouyu"))
            return 3;
        else
            return 2;
    }
};

YJCM2012Package::YJCM2012Package()
    :Package("YJCM2012")
{
    General *wangyi = new General(this, "wangyi", "wei", 3, false);
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3);
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    General *caozhang = new General(this, "caozhang", "wei");
    caozhang->addSkill(new Jiangchi);
    caozhang->addSkill(new JiangchiForbid);
    related_skills.insertMulti("jiangchi", "#jiangchi_forbid");
    skills << new JiangchiSlash;

    General *madai = new General(this, "madai", "shu");
    madai->addSkill(new Qianxi);
    madai->addSkill("mashu");

    General *liaohua = new General(this, "liaohua", "shu");
    liaohua->addSkill(new Dangxian);
    liaohua->addSkill(new MarkAssignSkill("@laoji", 1));
    liaohua->addSkill(new Fuli);

    General *guanxingzhangbao = new General(this, "guanxingzhangbao", "shu");
    guanxingzhangbao->addSkill(new Fuhun);

    General *chengpu = new General(this, "chengpu", "wu");
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new Chunlao);
    skills << new LihuoSlash;

    General *bulianshi = new General(this, "bulianshi", "wu", 3, false);
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *handang = new General(this, "handang", "wu");
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);
    handang->addSkill(new MarkAssignSkill("@bother", 1));
    skills << new GongqiSlash;

    General *liubiao = new General(this, "liubiao", "qun", 4);
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Zongshi);
	
    General *huaxiong = new General(this, "huaxiong", "qun", 6);
    huaxiong->addSkill(new Shiyong);

    addMetaObject<MijiCard>();
    addMetaObject<QiceCard>();
    addMetaObject<ChunlaoCard>();
    addMetaObject<AnxuCard>();
    addMetaObject<GongqiCard>();
    addMetaObject<JiefanCard>();
}

ADD_PACKAGE(YJCM2012)
